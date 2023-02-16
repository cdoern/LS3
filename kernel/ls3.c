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
#include <linux/delay.h>


// this pr_fmt macro causes printk to prefix every
// print statement with "ls3" and a function name
#undef pr_fmt // eliminate warning about redefining macro
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#define TRACE_SLOW() do { pr_info("at line %d\n", __LINE__); msleep(1000); } while (0)

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

// Helper onstants for MB, GB, and a reasonable minimum backing file size
#define MiB  (1024*1024L)
#define GiB  (1024*MiB)
#define MIN_BACKING_FILE_SIZE (1ULL*MiB)

// Layout of data on disk (SSD) is a series of records, starting at offset 0,
// and ending with an invalid record. A valid record looks like:
//
// +-- 8 bytes --+-- 8 bytes --+------ X bytes --------+------ Y bytes -----+
// |  uint64 X   |  uint64 Y   | ascii, non-terminated |      raw bytes     |
// +-- key_len --+- value_len -+-------- key ----------+------- value ------+
//
// A valid, full record will have
//    key_len > 0
// A valid but empty record will have
//    key_len == 0, value_len > 0
// A record is invalid when
//    key_len == 0, value_len == 0
//
// The minimum record size is 17:
//   8 for key_len, 8 for value_len, 1 for key, 0 for value.
//
// Typical layout of multiple records on disk (i.e. within the backing file):
// 0                                                    end_pos      backing_size
// v                                                     V                      V
// +--------+--------+--------+--------+--------+--------+---------+-----------+
// |  full  |  full  |  empty |  full  |  empty |  full  | invalid | ignored   |
// +--------+--------+--------+--------+--------+--------+---------+-----------+
//
// Another possible layout, when disk is nearly full
// 0                                                               backing_size
// v                                                                 end_pos  V
// +--------+--------+--------+--------+--------+------+------+----------+-----+
// |  full  |  full  |  empty |  full  |  empty | full | full |   full   |extra|
// +--------+--------+--------+--------+--------+------+------+----------+-----+
//
// Some invariants:
//
//  - Valid records are consecutive on disk from offset 0 to end_pos-1. So
//    end_pos is one past the last valid record, pointing at either the first
//    invalid record, or a small amount of extra space near the end of the
//    backing file, or the end of the backing file itself.
//
//  - Everything following an invalid record is ignored. This means there can be
//    at most one invalid record, and it is the last of the records.
//
//  - If there are fewer than 17 bytes following a valid record, it is
//    considered "extra" and ignored.
//
//  - If a record overflows the end of the backing file, this is an error. We
//    just ignore that record entirely.
//
//  - The last valid record will never be empty. So an empty record will always
//    be followed by a full record. If the last valid record (which is
//    necessarily full) is deleted, rather than marking it as empty, the end_pos
//    is moved up instead, and if there is room, an invalid record is written
//    there.
//
//  - There are never two adjacent empty records. They are always merged
//    together to form a single empty record instead.

struct record_hdr {
    uint64_t key_len;
    uint64_t value_len;
};
#define RECORD_HDR_SIZE (16)  /* sizeof(struct record_hdr) */
#define RECORD_MIN_SIZE (17)  /* sizeof(struct record_hdr) + 1 byte for a key */

struct record_info { // useful for iteration
    loff_t start, next;
    struct record_hdr hdr;
};


struct ioctl_data {
    uint64_t key_len;
    uint64_t value_len;
    void __user* key_uptr;
    void __user* value_uptr;
};

// Most global state is stored in the mount_info data structure. This
// version of the code only supports one mount point at a time, but a
// filesystem-registered version should be able to support multiple mount
// points, each using a different backing file, different sizes, different end
// positions, etc.
struct mount_info {
    loff_t end_pos;
    loff_t backing_size;
    struct file *filp;
};
struct mount_info ls3;


// Variable to track whether the init code successfully registered the module.
int registered = 0;

// Which (single) backing file to use. This should probably go in mount_info.
// This can be set by an insmod parameter.
static char* backing_file; // e.g. "/home/charliedoern/Documents/testing.txt"

// Global variable to control how much printing.
// This can be set by an insmod parameter.
static int verbose = 0;


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Doern");
MODULE_DESCRIPTION("LS3 Block Storage");
module_param(backing_file, charp, 0644);
MODULE_PARM_DESC(backing_file, "Path to backing file or SSD");
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Amount of debug printing");

static int rec_full(struct record_info *r) {
    return r->hdr.key_len > 0;
}

static int rec_empty(struct record_info *r) {
    return r->hdr.key_len == 0 && r->hdr.value_len > 0;
}

static int rec_invalid(struct record_info *r) {
    return r->hdr.key_len == 0 && r->hdr.value_len == 0;
}

static int rec_valid(struct record_info *r) {
    return r->hdr.key_len > 0 || r->hdr.value_len > 0;
}

static uint64_t rec_size(struct record_info *r) {
    return RECORD_HDR_SIZE + r->hdr.key_len + r->hdr.value_len;
}

// note: attempting to get a record beyond end_pos will not read, but instead
// will return an invalid record.
static void rec_get(loff_t pos, struct record_info *r) {
    r->start = pos;
    if (pos + RECORD_MIN_SIZE <= ls3.end_pos) 
        kernel_read(ls3.filp, &r->hdr, RECORD_HDR_SIZE, &pos);
    else
        r->hdr.key_len = r->hdr.value_len = 0;
    r->next = pos + r->hdr.key_len + r->hdr.value_len;
}

static char* rec_read_key(struct record_info *r) {
    char *key = kmalloc(r->hdr.key_len+1, GFP_USER);
    loff_t pos = r->start + RECORD_HDR_SIZE;
    kernel_read(ls3.filp, key, r->hdr.key_len, &pos);
    key[r->hdr.key_len] = '\0';
    return key;
}

static char* rec_read_value(struct record_info *r, loff_t offset, uint64_t len) {
    char *value = kmalloc(len, GFP_USER);
    loff_t pos = r->start + RECORD_HDR_SIZE + r->hdr.key_len + offset;
    kernel_read(ls3.filp, value, len, &pos);
    return value;
}


// Determine offset of first invalid record, which marks the end of the sequence
// of valid (full or empty) records. Also perform some sanity checks, e.g. to
// ensure the last valid record doesn't overflow the backing file.
static loff_t scan_for_end(void) {
    struct record_info r;
    ls3.end_pos = ls3.backing_size; // needed for iteration functions
    for (rec_get(0, &r); rec_valid(&r); rec_get(r.next, &r)) {
        if (verbose > 0)
            pr_info("Found valid record at offset %lld\n", r.start);
        // sanity checks before moving to next record
        if (r.next > ls3.backing_size) {
            pr_err("Last valid record overflows backing file by %lld bytes\n",
                    r.next - ls3.backing_size);
            pr_warn("Ignoring overflowing record\n");
            return r.start;
        }
        if (r.next + RECORD_MIN_SIZE >= ls3.backing_size) {
            // we can't go any further without overflowing backing file
            if (verbose > 0)
                pr_info("No invalid records found, end is at offset %lld\n", r.next);
            return r.next;
        }
    }
    if (verbose > 0)
        pr_info("First invalid record at offset %lld\n", r.start);
    return r.start;
}

// Write a new full record at given offset.
// - If extra >= RECORD_MIN_SIZE, then a new empty record is also written after
//   the record. This occurs when a hole is being partially filled, and there is
//   enough space leftover to make a new hole. Note: by the invariants, the hole
//   being filled must be followed by a full record, so we don't need to worry
//   about having the newly created hole be followed by a subsequent hole, or
//   having the newly created hole be followed by the end_pos, both of which
//   would violate the invariants.
// - If extra == 0, then nothing is written after the record. This occurs when a
//   hole is being completely filled, or when a new record is being placed
//   at end_pos.
// - Any other value for extra is not allowed.
static void rec_write(loff_t off,
        uint64_t key_len, uint64_t value_len,
        char *key, void *value,
        uint64_t extra) /* extra must be 0, or at least RECORD_MIN_SIZE */
{
    struct record_hdr hdr;
    hdr.key_len = key_len;
    hdr.value_len = value_len;
    kernel_write(ls3.filp, &hdr, RECORD_HDR_SIZE, &off);
    kernel_write(ls3.filp, key, key_len, &off);
    kernel_write(ls3.filp, value, value_len, &off);
    if (extra >= RECORD_MIN_SIZE) {
        hdr.key_len = 0;
        hdr.value_len = extra - RECORD_HDR_SIZE;
        kernel_write(ls3.filp, &hdr, RECORD_HDR_SIZE, &off);
    }
}

static int find_key(char *key, uint64_t key_len, struct record_info *prev, struct record_info *curr) {
    prev->hdr.key_len = prev->hdr.value_len = 0;
    for (rec_get(0, curr); rec_valid(curr); rec_get(curr->next, curr)) {
        if (rec_empty(curr) ||curr->hdr.key_len != key_len)
            continue;

        char *curr_key = rec_read_key(curr);
        bool match = (strcmp(curr_key, key) == 0);
        kfree(curr_key);
        curr_key = NULL;
        
        if (match)
            return 1;
        if (prev)
            *prev = *curr;
    }
    return 0;
}

static int ls3_ioctl_getobject(struct ioctl_data *params, char *key, void __user* param_uptr)
{
    struct record_info r;
    if (!find_key(key, params->key_len, NULL, &r)) {
        pr_info("Failed to find matching key\n");
        return -ENOENT;
    }
    // match: read value, copy value_len and cur_value to user
    // only copy lesser amount of actual value size and user buffer size
    uint64_t amt = r.hdr.value_len;
    if (amt > params->value_len) {
        pr_info("Returning %lld of %lld bytes\n", amt, r.hdr.value_len);
        amt = params->value_len;
    }
    if (amt > 0) {
        void *curr_value = rec_read_value(&r, 0, amt);
        if (copy_to_user(params->value_uptr, curr_value, amt)) {
            pr_err("Failed to copy ioctl result data to userspace\n");
            kfree(curr_value);
            return -EFAULT;
        }
    }
    // copy the actual value size to the user ioctl header
    if (copy_to_user(param_uptr + offsetof(struct ioctl_data, value_len),
                &r.hdr.value_len, sizeof(r.hdr.value_len))) {
        pr_err("Failed to copy ioctl result size to userspace\n");
        return -EFAULT;
    }
    return 0;
}

// note: this doesn't erase the data on disk, it only overwrites the headers so
// that the data is no longer pointed to by a valid record
static int ls3_ioctl_delobject(struct ioctl_data *params, char *key)
{
    struct record_info prev, curr, next, *dirty = NULL;
    if (!find_key(key, params->key_len, &prev, &curr)) {
        pr_info("Failed to find matching key\n");
        return -ENOENT;
    }

    rec_get(curr.next, &next);
  
    if (rec_empty(&prev) && rec_empty(&next)) {
        // prev     curr         next
        // [empty]  [to-delete]  [empty]
        // merge all three into a single empty record
        prev.hdr.value_len += RECORD_HDR_SIZE + curr.hdr.key_len + curr.hdr.value_len;
        prev.hdr.value_len += RECORD_HDR_SIZE + next.hdr.value_len;
        dirty = &prev;
    } else if (rec_empty(&prev) && rec_full(&next)) {
        // prev     curr         next
        // [empty]  [to-delete]  [full]
        // merge two into a single empty record
        prev.hdr.value_len += RECORD_HDR_SIZE + curr.hdr.key_len + curr.hdr.value_len;
        dirty = &prev;
    } else if (rec_empty(&prev) && rec_invalid(&next)) {
        // prev     curr         next
        // [empty]  [to-delete]  [invalid]
        // eliminate all three, and adjust end_pos
        prev.hdr.key_len = prev.hdr.value_len = 0;
        ls3.end_pos = prev.start;
        dirty = &prev;
    } else if (rec_empty(&next)) {
        // prev    curr         next
        // [full]  [to-delete]  [empty]
        // merge two into a single empty record
        curr.hdr.value_len += curr.hdr.key_len;
        curr.hdr.key_len = 0;
        curr.hdr.value_len += RECORD_HDR_SIZE + next.hdr.value_len;
        dirty = &curr;
    } else if (rec_invalid(&next)) {
        // prev    curr         next
        // [full]  [to-delete]  [invalid]
        // eliminate two, and adjust end_pos
        curr.hdr.key_len = curr.hdr.value_len = 0;
        ls3.end_pos = curr.start;
        dirty = &curr;
    } else {
        // prev    curr         next
        // [full]  [to-delete]  [full]
        // make only curr into a new empty record
        curr.hdr.value_len += curr.hdr.key_len;
        curr.hdr.key_len = 0;
        dirty = &curr;
    }

    // overwrite the dirty record
    loff_t pos = dirty->start;
    kernel_write(ls3.filp, &dirty->hdr, RECORD_HDR_SIZE, &pos);
    return 0;
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

    // TODO: many optimizations possible here...
    // - put new value in place of old value if same size
    // - put new value plus a hole in place of old value if new value is smaller
    // - put new value in place of a preceeding hole plus old value if it fits
    // - put new value in place of a old value plus a following hole if it fits
    // - single scan: find a suitable hole while searching for old value
    // - hole selection: best-fit vs next-fit vs worst-fit vs random, etc.
   
    int err = 0;

    // Scan to find previous copy of key, if present, and delete it
    ls3_ioctl_delobject(params, key);

    // Then scan again to find first-fit hole.
    uint64_t amt = RECORD_HDR_SIZE + params->key_len + params->value_len;
    struct record_info curr, prev = { 0 };
    for (rec_get(0, &curr); rec_valid(&curr); rec_get(curr.next, &curr)) {
        if (rec_empty(&curr) && (rec_size(&curr) == amt || rec_size(&curr) >= amt + RECORD_MIN_SIZE))
            break;
        prev = curr;
    }
    if (rec_empty(&curr)) {
        // new record fits exactly into existing hole, or
        // fits with enough leftover to make a new hole
        uint64_t hole_size = rec_size(&curr);
        uint64_t extra = hole_size - amt;
        // Note: rec_write will also make a new hole after the record, if needed.
        rec_write(curr.start, params->key_len, params->value_len, key, value, extra);
    } else if (curr.start + amt <= ls3.backing_size) {
        // no hole found, but new record fits near end of file
        // assert(curr.start == end_pos)
        rec_write(curr.start, params->key_len, params->value_len, key, value, 0);
        ls3.end_pos = curr.start + amt;
        uint64_t extra = ls3.backing_size - ls3.end_pos;
        if (extra > 0) {
            // write a complete or partial invalid header, which is just all zeros
            struct record_hdr hdr = { 0 };
            loff_t pos = ls3.end_pos;
            if (extra > RECORD_HDR_SIZE)
                extra = RECORD_HDR_SIZE;
            kernel_write(ls3.filp, &hdr, extra, &pos);
        }
    } else {
        pr_err("No space left in backing store, or too much fragmentation.");
        // TODO: keep track of some statistics? E.g. bytes used, wasted space,
        // num key/value pairs, avg size of keys, etc.
        err = -ENOSPC;
    }

    kfree(value);
    return err;
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
        p++;
    }
    return 0;
}
#endif // ENFORCE_ASCII_KEY_NAMES
    

static int ls3_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if (!registered) {
        pr_err("ls3 not yet initialized");
        return -ENOENT;
    }

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
        pr_info("key [len=%lld]: %s", ioctl_params.key_len, key);
#else
    if (verbose > 0 && validate_key_name(key))
        pr_info("key [len=%lld] \"%s\"", ioctl_params.key_len, key);
    else if (verbose > 0)
        pr_info("key [len=%lld] will not be printed\n", ioctl_params.key_len);
#endif // ENFORCE_ASCII_KEY_NAMES

    int err;
    switch(cmd) {

        case LS3_IOCTL_CMD_PUTOBJECT:
            if (verbose > 0)
                pr_info("PutObject with %lld byte value\n", ioctl_params.value_len);
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

int __init main(void) {
    pr_info("Initializing LS3 filesystem\n");
    memset(&ls3, 0, sizeof(struct mount_info));
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
    ls3.filp = filp_open(backing_file, O_RDWR, 0644);
    if (IS_ERR_OR_NULL(ls3.filp)) {
        pr_err("Failed to initialize: can't open '%s'\n", backing_file);
        ls3.filp = NULL;
        return -EINVAL;
    }
    if (verbose > 0) pr_info("Opened backing file\n");

    loff_t size = i_size_read(file_inode(ls3.filp));
    if (size < MIN_BACKING_FILE_SIZE) {
        pr_err("Backing file has size %lld, but must be %lld at minimum\n",
                size, MIN_BACKING_FILE_SIZE);
        if (verbose > 0) pr_info("Closing backing file\n");
        filp_close(ls3.filp, NULL);
        ls3.filp = NULL;
        return -EIO;
    }
    if (verbose > 0)
        pr_info("Backing file has size %lld bytes (%lld.%03lld %s)\n",
                size,
                size > 1*GiB ? size/GiB : size/MiB,
                size > 1*GiB ? (size%GiB)/(GiB/1000) : (size%MiB)/(MiB/1000),
                size > 1*GiB ? "GiB" : "MiB");
    ls3.backing_size = size;

    if (verbose > 0) pr_info("Scanning for end position\n");
    ls3.end_pos = scan_for_end();

    if (verbose > 0) pr_info("Registering module\n");
    int err = misc_register(&ls3_device);
    if (err != 0) {
        pr_err("Failed to register: err=%d\n", err);
        if (verbose > 0) pr_info("Closing backing file\n");
        filp_close(ls3.filp, NULL);
        ls3.filp = NULL;
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
    if (!IS_ERR_OR_NULL(ls3.filp)) {
        if (verbose > 0) pr_info("Closing backing file\n");
        filp_close(ls3.filp, NULL);
        ls3.filp = NULL;
    }

    if (registered) {
        if (verbose > 0) pr_info("Unregistering module\n");
        misc_deregister(&ls3_device);
        registered = 0;
    }

    pr_info("Completed cleanup\n");
}

module_init(main)
module_exit(cleanup)
