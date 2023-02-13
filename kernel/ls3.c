#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <asm/unistd.h>
#include <linux/uaccess.h>
#include <linux/socket.h>
#include <linux/if.h>
#include <linux/net.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/processor.h>
#include <linux/stddef.h>
#include <linux/uaccess.h>


// this pr_fmt macro causes printk to prefix every
// print statement with "ls3" and a function name
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

// The ioctl syscall is the primary userspace entrypoint to ls3.
// FIXME: where did these numbers come from?
#define LS3_IOCTL_CMD_GETOBJECT (0x0001) // IOCTL_CMD_READ?
#define LS3_IOCTL_CMD_PUTOBJECT (0x0707) // IOCTL_CMD_RDWR?
#define LS3_IOCTL_CMD_DELOBJECT (0x8000) // IOCTL_CMD_DELETE?
// Keys must be between 1 and 127 bytes.
// Empty keys (len=0) are not allowed.
#define LS3_MAX_KEYLEN (127)
// Values can be any non-negative length.
// NOTE: zero-length values are allowed!

// Amazon S3 recommends (but does not require) using only certain safe
// characters in key names. For now, ls3 can (optionally) enforce that only
// printable ascii is used in key names, which mostly (but not exactly) matches
// the Amazon S3 recommendations.
#define ENFORCE_ASCII_KEY_NAMES

struct ioctl_data {
    uint64_t key_len;
    uint64_t value_len;
    void __user* key_uptr;
    void __user* value_uptr;
};

struct appendable_data {
    uint64_t key_len;
    uint64_t value_len;
    char *key;
    char *value; // change to void*
};

struct appendable_data all_data[1000000];
int tracker = 0;
loff_t end_pos = 0;
loff_t write_pos = 0;
uint64_t leftover = 0;
struct file *filp = NULL;
int registered = 0;

static char* backing_file; // e.g. "/home/charliedoern/Documents/testing.txt"
static int verbose = 0; // controls how much printing

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Doern");
MODULE_DESCRIPTION("LS3 Block Storage");
module_param(backing_file, charp, 0644);
MODULE_PARM_DESC(backing_file, "Path to backing file or SSD");
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Amount of debug printing");

// FIXME: eliminate
static int write_array(char *key, char *value, uint64_t key_len, uint64_t value_len) {
    struct appendable_data append;
    append.key = key;
    append.value = value;
    append.key_len = key_len;
    append.value_len = value_len;
    int loop;
    int place = tracker;
    for(loop = 0; loop < tracker; loop++) {
        printk(KERN_INFO "key, value: %s, %s\n", all_data[loop].key, all_data[loop].value);
        if (all_data[loop].key == NULL) {
            place = loop;
        } else if (strcmp(all_data[loop].key, key) == 0) {
            place = loop;
            break;
        }
    }
    all_data[place] = append;
    return place;
}

static loff_t scan_for_end(void) {
    // look for blank spot
    // check sizes using the read size and if not big enough, skip
    // when deleting, I kept the lenghts but deleted the data
    uint64_t key_len;
    uint64_t value_len;
    loff_t readPos = 0;
    while(true) {
        printk(KERN_INFO "%d\n", (int)readPos);
        loff_t beginning = readPos;
        int ret = kernel_read(filp, &key_len, 8, &readPos);
        ret = kernel_read(filp, &value_len, 8, &readPos);
        if (key_len == 0 && value_len == 0) {
            readPos -= 16;
            break;
        }
        // ret = kernel_read(filp, curr_value, value_len, &write_pos);
        readPos += key_len+value_len;
    }
    return readPos;
}

static int look_for_blanks(int place, struct ioctl_data *data) {
    // look for blank spot
    // check sizes using the read size and if not big enough, skip
    // when deleting, I kept the lenghts but deleted the data
    uint64_t key_len;
    uint64_t value_len;
    loff_t readPos = 0;
    int ret = 0;
    while(readPos < end_pos) {
        leftover = 0;
        printk(KERN_INFO "%d\n", (int)readPos);
        loff_t beginning = readPos;
        ret = kernel_read(filp, &key_len, 8, &readPos);
        ret = kernel_read(filp, &value_len, 8, &readPos);
        if ((key_len+value_len) < (data->key_len+data->value_len)) {
            readPos += key_len+value_len;
            continue;
        } else if ((key_len+value_len) - 17 > (data->key_len+data->value_len)) {
            // good w/ chunk
            leftover = (key_len+value_len) - (data->key_len+data->value_len);
        }  else if ((key_len+value_len) == (data->key_len+data->value_len)) {
            // good perfectly
            // do nothing
        }   else {
            // bad
            readPos += key_len+value_len;
            continue;
        }
        char *curr_key = kmalloc(1, GFP_USER);
        ret = kernel_read(filp, curr_key, 1, &readPos);
        readPos += key_len+value_len-1;
        //char *zeros = kmalloc(key_len+1, GFP_USER);
        // memset(zeros, '0', key_len);
        //zeros[key_len] = 0;
        if (curr_key[0] == 0) {
            write_pos = beginning; // or something? make a string of zeros and if our key is that then we got a writing place
            break;
        }
        // ret = kernel_read(filp, curr_value, value_len, &write_pos);
    }
    return ret;
}

static void write_file(int place) {
    if (filp) {
        printk(KERN_INFO, "success\n");
        int size = all_data[place].key_len + all_data[place].value_len + sizeof(uint64_t)*2;
        char* appendFile = kmalloc(size, GFP_USER);
        //appendFile[0] = 0;
        memcpy(appendFile, &all_data[place].key_len, 8);
        memcpy(appendFile+sizeof(uint64_t), &all_data[place].value_len, 8);
        memcpy(appendFile+2*sizeof(uint64_t), all_data[place].key, all_data[place].key_len);
        memcpy(appendFile+2*sizeof(uint64_t)+all_data[place].key_len, all_data[place].value, all_data[place].value_len);
        printk(KERN_INFO, "here, %s", appendFile);
        //strcat(append, " ");
        //strcat(append, all_data[place].value); // memcopy instead
        // char* append = (all_data[place].key + " " + all_data[place].value);
        void* key_mem_addr = NULL;
        key_mem_addr = appendFile;
        //   void* mem_addr = all_data[place].value;
        if (write_pos != end_pos) {
            kernel_write(filp, key_mem_addr, size, &write_pos);
            if (leftover != 0) {
                char *chunk = kmalloc(17, GFP_USER);
                uint64_t zero = 0;
                memcpy(chunk, &leftover, 8);
                memcpy(chunk+8, &zero, 8);
                memcpy(chunk+16, &zero, 1);
                void* chunk_mem_addr = chunk;
                kernel_write(filp, chunk_mem_addr, 17, &write_pos);
            }
        } else {
            kernel_write(filp, key_mem_addr, size, &end_pos);
        }
        //pos = pos+(size+1);
    }
}

static int read_file(struct ioctl_data *data, char *key) {
    if (filp) {
        printk(KERN_INFO, "success\n");
        //strcat(append, all_data[place].value); // memcopy instead
        // char* append = (all_data[place].key + " " + all_data[place].value);
        //   void* mem_addr = all_data[place].value;
        ssize_t ret;
        uint64_t key_len;
        uint64_t value_len;
        loff_t readPos = 0;
        while(readPos < end_pos) {
            printk(KERN_INFO "%d\n", (int)readPos);
            ret = kernel_read(filp, &key_len, 8, &readPos);
            ret = kernel_read(filp, &value_len, 8, &readPos);
            char *curr_key = kmalloc(key_len+1, GFP_USER);
            void *curr_value = kmalloc(value_len, GFP_USER);
            printk(KERN_INFO "lengths %llu %llu\n", key_len, value_len);
            ret = kernel_read(filp, curr_key, key_len, &readPos);
            curr_key[key_len] = 0;
            if ((strcmp(curr_key, key) != 0)) {
                readPos = readPos + value_len;
                continue;
            } 
            ret = kernel_read(filp, curr_value, value_len, &readPos);
            data->value_len = value_len;
            if (copy_to_user(data->value_uptr, curr_value, value_len)) {
                return -EFAULT;
            }
            return 0;
        }
        return -ENOENT;
        //pos = pos+(size+1);
    }
    return -EBADF;
}

static int ls3_ioctl_putobject(struct ioctl_data *params, char *key)
{
    // copy params->val from userspace
    // NOTE: zero-length values are allowed.
    void* value = kmalloc(params->value_len, GFP_USER);
    if (copy_from_user(value, params->value_uptr, params->value_len)) {
        pr_err("Failed to copy val from userspace\n");
        kfree(value);
        return -EFAULT;
    }

    // FIXME: code cleanup needed here, and remove array
    // FIXME: add duplicate checking?
    int place = write_array(key, value, params->key_len, params->value_len);
    if (place < 0) {
        pr_err("Failed to write array\n");
        kfree(value);
        return -EFAULT;
    }
    write_pos = end_pos;
    leftover = 0;
    if (place == tracker) {
        tracker = tracker + 1;
    } else if (filp) {
        int ret = look_for_blanks(place, params);
        if (!ret) {
            pr_err("No blanks found\n");
            kfree(value);
            return -EFAULT;
        }
    }

    write_file(place);
    //char ioctl_params[ioctl_params.key_len+ioctl_params.value_len+1] = key + " " + value;

    kfree(value);
    return 0;
}

static int ls3_ioctl_getobject(struct ioctl_data *params, char *key, void __user* param_uptr)
{
    // FIXME: cleanup here
    //read_array(params, key);
    int err = read_file(params, key);
    if (err != 0) {
        return err;
    }
    // FIXME: do we only really need to copy the param->data_length field?
    if (copy_to_user(param_uptr, params, sizeof(struct ioctl_data))) {
        pr_err("Failed to copy ioctl result data to userspace\n");
        return -EFAULT;
    }
    return 0;
}

static int ls3_ioctl_delobject(struct ioctl_data *params, char *key)
{
    // FIXME: cleanup here
    int loop = 0;
    // we only need to get the key here now go look for the value
    for(loop = 0; loop < tracker; loop++) {
        printk(KERN_INFO "key %d of %d key %s\n", loop, tracker, all_data[loop].key);
        if (all_data[loop].key == NULL) {
            continue;
        }
        if(strcmp(all_data[loop].key, key) == 0) {
            printk(KERN_INFO "found key... deleting\n");
            all_data[loop].value = NULL;
            all_data[loop].key = NULL;
            break;
        }
    }

    ssize_t ret;
    uint64_t key_len;
    uint64_t value_len;
    loff_t readPos = 0;
    while(readPos < end_pos) {
        printk(KERN_INFO "%d\n", (int)readPos);
        ret = kernel_read(filp, &key_len, 8, &readPos);
        ret = kernel_read(filp, &value_len, 8, &readPos);
        char *curr_key = kmalloc(key_len+1, GFP_USER);
        void *curr_value = kmalloc(value_len, GFP_USER);
        printk(KERN_INFO "lengths %llu %llu\n", key_len, value_len);
        ret = kernel_read(filp, curr_key, key_len, &readPos);
        curr_key[key_len] = 0;
        if ((strcmp(curr_key, key) != 0)) {
            readPos = readPos + value_len;
            continue;
        } 
        int size = key_len + value_len;
        char* appendFile = kmalloc(size, GFP_USER);
        memset(appendFile, 0, size);  
        //appendFile[0] = 0;
        //qappendFile[size-1] = 0;
        void* key_mem_addr = NULL;
        key_mem_addr = appendFile;
        readPos -= key_len;
        kernel_write(filp, key_mem_addr, size, &readPos);
        return 0;
    }
    return 0; // ??? FIXME this case was missing before
}

#ifdef ENFORCE_ASCII_KEY_NAMES
static int validate_key_name(char *key) {
    char *p = key;
    if (!p) {
        return 1;
    }
    while (*p) {
        if (*p < 0x20 || *p > 0x7E)
            return 1;
    }
    return 0;
}
#endif // ENFORCE_ASCII_KEY_NAMES

static int ls3_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user* param_uptr = (void __user*)arg;
    if (verbose > 0) pr_info("ioctl(..., cmd=0x%04x, arg=%p)n", cmd, param_uptr);
    // Sanity check
    if (cmd == 0 || arg == 0) {
        pr_err("Invalid cmd or arg\n");
        return -EINVAL;
    }
    // copy ioctl parameter data from userspace
    struct ioctl_data ioctl_params;
    if (copy_from_user(&ioctl_params, (void __user*)arg, sizeof(ioctl_params))) {
        pr_err("Failed to copy ioctl parameter data from userspace\n");
        return -EFAULT;
    }
    // copy ioctl_params->key from userspace
    if (ioctl_params.key_len <= 0 || ioctl_params.key_len > LS3_MAX_KEYLEN) {
        pr_err("Invalid key length (len=%llu)\n", ioctl_params.key_len);
        return -EINVAL;
    }
    char *key = kmalloc(ioctl_params.key_len+1, GFP_USER);
    if (key == NULL)
        return -ENOMEM;
    if (copy_from_user(key, ioctl_params.key_uptr, ioctl_params.key_len)) {
        pr_err("Failed to copy key from userspace\n");
        kfree(key);
        return -EFAULT;
    }
    key[ioctl_params.key_len] = 0;

#ifdef ENFORCE_ASCII_KEY_NAMES
    if (validate_key_name(key)) {
        pr_err("Invalid key, does not contain only printable ascii\n");
        kfree(key);
        return -EINVAL;
    }
    if (verbose > 0)
        pr_info("key [len=%d]: %s", ioctl_params.key_len, key);
#else
    if (verbose > 0 && validate_key_name(key))
        pr_info("key [len=%d] \"%s\"", ioctl_params.key_len, key);
    else if (verbose > 0)
        pr_info("key [len=%d] will not be printed\n", ioctl_params.key_len);
#endif // ENFORCE_ASCII_KEY_NAMES

    int err;
    switch(cmd) {

        case LS3_IOCTL_CMD_PUTOBJECT:
            if (verbose > 0)
                pr_info("PutObject with %d byte value\n", ioctl_params.value_len);
            err = ls3_ioctl_putobject(&ioctl_params, key);
            break;

        case LS3_IOCTL_CMD_GETOBJECT:
            if (verbose > 0)
                pr_info("GetObject\n");
            err = ls3_ioctl_getobject(&ioctl_params, key, param_uptr);
            break;

        case LS3_IOCTL_CMD_DELOBJECT:
            if (verbose > 0)
                pr_info("DeleteObject\n");
            err = ls3_ioctl_delobject(&ioctl_params, key);
            break;

        default:
            pr_err("Unrecognized ioctl (cmd=0x%04x)\n", cmd);
            err = -EINVAL;
    }

    kfree(key);
    return err;
}

static int ls3_open(struct inode *inode, struct file *file)
{
    return 0; // TODO
}

static int ls3_release(struct inode *inode, struct file *file)
{
    return 0; // TODO
}

//populate data struct for file operations
static const struct file_operations ls3_fops = {
    .owner = THIS_MODULE,
    .open = ls3_open,
    // .read = lfs_read_file, // TODO
    // .write = lfs_write_file, // TODO
    .release = &ls3_release,
    // .compat_ioctl = (void*)&ls3_ioctl, // Do not support 32-bit ioctls
    .unlocked_ioctl = (void*)&ls3_ioctl
};

//populate miscdevice data structure
static struct miscdevice ls3_device = {
    MISC_DYNAMIC_MINOR,
    "ls3",
    &ls3_fops,
    .mode = S_IRWXUGO
};

static int __init main(void) {
    pr_info("Initializing LS3 filesystem\n");
    if (backing_file == NULL) {
        pr_err("Failed to initialize: backing_file is NULL\n");
        return -EINVAL;
    }
    if (backing_file[0] == '\0') {
        pr_err("Failed to initialize: backing_file is emptystring\n");
        return -EINVAL;
    }
    pr_info("Using backing_file '%s'\n", backing_file);
    // todo: use chmod to make file world readable?
    filp = filp_open(backing_file, O_RDWR, 0644);
    if (IS_ERR_OR_NULL(filp)) {
        pr_err("Failed to initialize: can't open '%s'\n", backing_file);
        filp = NULL;
        return -EINVAL;
    }
    if (verbose > 0) pr_info("Opened backing file\n");

    if (verbose > 0) pr_info("Scanning for end position\n");
    end_pos = scan_for_end();

    if (verbose > 0) pr_info("Registering module\n");
    int err = misc_register(&ls3_device);
    if (err != 0) {
        pr_err("Failed to register: err=%d\n", err);
        if (verbose > 0) pr_info("Closing backing file\n");
        filp_close(filp, NULL);
        filp = NULL;
        return err;
    }
    registered = 1;

    // TODO: register as a filesystem
    // err = register_filesystem(&lfs_type);

    return 0;
}

static void __exit cleanup(void)
{
    pr_info("Cleaning up LS3 filesystem\n");
    if (!IS_ERR_OR_NULL(filp)) {
        if (verbose > 0) pr_info("Closing backing file\n");
        filp_close(filp, NULL);
        filp = NULL;
    }

    if (registered) {
        if (verbose > 0) pr_info("Unregistering module\n");
        misc_deregister(&ls3_device);
    }

    pr_info("Completed cleanup\n");
}

module_init(main)
module_exit(cleanup)
