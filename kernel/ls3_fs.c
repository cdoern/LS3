// need to design superblock 
// and fs before garbage collection
// because sure we have keys and data but how do we go about
// finding that data w/in the fs?
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
#include <linux/dcache.h>
#include <linux/bitmap.h>

#define LS3_MAGIC   0x858458f6
#define PAGE_CACHE_SIZE 8192
#define PAGE_CACHE_SHIFT 13


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Charles Doern");
MODULE_DESCRIPTION("LS3 Block Storage");
struct dentry *ls3_mount(struct file_system_type *ls3_type, int flags, const char *dev_name, void *data);
static int ls3_fill_super (struct super_block *sb, void *data, int silent);
void kill_ls3_super(struct super_block *sb);
static void get_block(uint64_t block, void* anything, uint64_t mountno);
static const struct file_operations my_fops;


#define __round_mask(x, y) ((__typeof__(x))((y)-1))

#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)


struct ioctl_data {
uint64_t cmd; 
uint64_t key_len;
uint64_t value_len;
void __user* key;
void __user* value;
uint64_t mountno;
};

/*
struct ioctl_mount {
uint64_t cmd; 
void __user* device;
int len_device_name;
};
*/

struct data_block {
//uint64_t data_len;
char value[8192-8];
uint64_t next_blockno;
};


struct key_entry { // 144 bytes per struct
char key[128];
uint64_t data_len; // bytes
uint64_t data_blockno;
};

struct key_block {
uint64_t older_blockno;
struct key_entry entry[56];
char unused[120];
};


struct superblock { // 0 - 31 super blocks. bitmap is (32 - N) * 2 where N is dependent on FS size / 8192 rounded to nearest mult of 8192
	uint64_t magic;
	uint64_t master; // (32 - N) * 2 - 1
	uint64_t master_list_len; // 1
	uint64_t fs_size; // size of "file"
	uint64_t timestamp; // current time
	uint64_t used_bitmaps[1018]; //32 - N 
};

// need to modify free bitmap where things are used initially

struct mount_data {
    struct superblock *super;
    struct file *filp;
    unsigned long *bitmap;
};

struct mount_data *mounts[10];
int num_mounts = 10;


static int garbage_collection(void) {
// start at some point in the fs

//uint64_t super_block_head = read_super_block_master(); // this will query s_fs_info and get the last key block

uint64_t prev = 0;

do {
	int i;
    for (i = 0; i < 55; i++) {
        // read key
        // increment loc
    }
    prev =  0;// where we are, jump to this block

} while (prev != 0);
return 0;
}

static struct file_system_type ls3_type = {
	.owner 	= THIS_MODULE,
	.name	= "ls3",
    .mount = ls3_mount,
	//.unmount = ls3_unmount
	//.get_sb	= ls3_get_super,
	// .parameters = fs_parameter_spec fs_parser.h
	.kill_sb = kill_ls3_super,
    };

static struct super_operations ls3_s_ops = {
    .statfs         = simple_statfs,
    .drop_inode     = generic_delete_inode,
};


void kill_ls3_super(struct super_block *sb)
{
    int mount;
    mount = (int)sb->s_fs_info;
    printk(KERN_INFO "closing filp %p", mounts[mount]->filp);
    filp_close(mounts[mount]->filp, NULL);
    mounts[mount] = NULL;
    kill_block_super(sb);

}
struct dentry *ls3_mount(struct file_system_type *ls3_type, int flags, const char *dev_name, void *data) {
    if (dev_name[0] != '\0') {

        //
        return mount_bdev(ls3_type, flags, dev_name, (void*)dev_name, ls3_fill_super);
    } else {
        return ERR_PTR(-EBADF);
    }
    // ??????????????????????????????????????????????????????????????
}
/*
struct dentry *ls3_unmount(struct file_system_type *ls3_type, int flags, const char *dev_name, void *data) {
            return unmount_nodev(ls3_type, flags, data, ls3_fill_super);
}
*/



// will change for our code
static int ls3_open(struct inode *inode, struct file *filp)
{
	/*
	    filp->private_data = inode->u.generic_ip;
	    return 0;
		*/

		return 0;
}

// will change for our code
static ssize_t ls3_read_file(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	return 0;
	/*char *pdata = (char *) file->private_data;
	int v, len;
	char tmp[TMPSIZE];
	len = snprintf(tmp, TMPSIZE, "%s\n", pdata);
	if (*offset > len)
		return 0;
	if (size > len - *offset)
		size = len - *offset;
	if (copy_to_user(data, tmp + *offset, pdata))
		return -EFAULT;
	*offset += size;
	return size;*/
}

// will change for our code
static ssize_t ls3_write_file(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	return 0;
	/*char *pdata = (char *) filp->private_data;
	char tmp[TMPSIZE];
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, data, size))
		return -EFAULT;
    pdata = tmp
	return count;*/
}

static struct inode *ls3_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_mode = mode;
		ret->i_uid.val = (uid_t)0;
		ret->i_gid.val = (gid_t)0;
		//ret->i_blksize = PAGE_CACHE_SIZE;
		ret->i_blocks = 0;
		//ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;
	}
	return ret;
}


// will change for our code
static struct dentry *ls3_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		char *data)
{
	return 0;
	/*
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);

	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = ls3_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &my_fops;
	inode->u.generic_ip = data;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;*/
}

static atomic_t data, subcounter;

/*
static void ls3_create_files (struct super_block *sb, struct dentry *root)
{
	struct dentry *subdir;

    char *testing = "hello world"
	ls3_create_file(sb, root, "testing_file", &testing);
}
*/


static uint64_t round_to_multiple(uint64_t number, uint64_t up_to) {
    return ((number + (up_to - 1) / up_to) * up_to);
}

static int ls3_fill_super (struct super_block *sb, void *data, int silent)
{
    printk(KERN_INFO "filling super");

    uint64_t mountno = -1;
    int i;
    for(i = 0; i < num_mounts; i++) {
        if (mounts[i] == NULL) {
            mountno = (uint64_t)i;
            break;
        }
    }
    if (i == num_mounts) {
        printk(KERN_INFO "MOUNT TABLE FULL");
        return -1;
    }

    
    //num_mounts++;
    mounts[mountno] = kzalloc(sizeof(struct mount_data), GFP_USER);
    printk(KERN_INFO "filp name  %s", (char*)data);
    mounts[mountno]->filp = filp_open((char*)data, O_RDWR, 0644);
    mounts[mountno]->super = kzalloc(sizeof(struct superblock), GFP_USER);
    // read block 0 from the file into data structure
    get_block(0, mounts[mountno]->super, mountno);

    uint64_t total_blocks =  mounts[mountno]->super->fs_size / PAGE_CACHE_SIZE; //+ 1; // plus one?

    //loff_t size_in_bytes = i_size_read(file_inode(new_mount.filp));
        //  loff_t num_blocks = size_in_bytes/PAGE_CACHE_SIZE; //+ 1; // WHAT IF SIZE IN BYTES < PAGE SIZE?
    loff_t num_freemap_bytes = (total_blocks + 7) / 8;
    loff_t num_freemap_blocks = (num_freemap_bytes + 8191) / 8192;
    uint64_t num_longs = ((total_blocks + (sizeof(unsigned long) * 8) - 1)) / (sizeof(unsigned long) * 8);
    uint64_t size_of_bitmap = num_longs * sizeof(unsigned long);
    mounts[mountno]->bitmap = kzalloc(round_to_multiple(size_of_bitmap, PAGE_CACHE_SIZE), GFP_USER);


    printk(KERN_INFO "MOUNT mounted FS: fs_size: %d, total blocks: %d number of freemap_bytes: %llu, number fo freemap blocks: %llu, size of bitmap: %d\n", 
    mounts[mountno]->super->fs_size, total_blocks, num_freemap_bytes, num_freemap_blocks, size_of_bitmap);
    printk(KERN_INFO "bitmap first block: %d", mounts[mountno]->super->used_bitmaps[0]);
    // should be a loop if bitmap size differs
    get_block(mounts[mountno]->super->used_bitmaps[0], mounts[mountno]->bitmap, mountno);
    printk(KERN_INFO " bitmap %p", mounts[mountno]->bitmap);
    // mounts[mountno]->super->master = 40;
    //mounts[mountno]->super->fs_size = i_size_read(file_inode(mounts[mountno]->filp));
    //mounts[mountno]->super->magic = LS3_MAGIC;
    if (mounts[mountno]->super->magic  != LS3_MAGIC) {
        printk(KERN_INFO "BAD MAGIC NO");
        num_mounts--;
        return NULL;
    }
    mounts[mountno]->super->master_list_len = 1;


    // why are we not formatting here?

    printk(KERN_INFO "Here!\n");
    
    printk(KERN_INFO "Here2!\n");

// we should rly be reading from disk here. not filling with zeros
    //bitmap_fill( mounts[mountno]->bitmap, total_blocks);
    for(i = 0; i < num_freemap_blocks; i++) {
        get_block(mounts[mountno]->super->used_bitmaps[i], ((void *)mounts[mountno]->bitmap)+i*PAGE_CACHE_SIZE, mountno);
    }
	struct inode *root;
	struct dentry *root_dentry;

	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = LS3_MAGIC;
	sb->s_op = &ls3_s_ops;
    sb->s_fs_info = (void*)mountno;
    //sb->s_maxbytes  = MAX_LFS_FILESIZE;

	root = ls3_make_inode(sb, S_IFDIR | 0755);
	if (! root)
		goto out;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_make_root(root);
	if (! root_dentry)
		goto out_iput;
	sb->s_root = root_dentry;

	//ls3_create_files (sb, root_dentry);
    printk(KERN_INFO "finished mounting %d", mountno);
	return 0;
	
  out_iput:
	iput(root);
  out:
	return -ENOMEM;
}


static int my_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
	return 0;
}


//populate miscdevice data structure

static struct miscdevice my_device = {
	MISC_DYNAMIC_MINOR,
	"ls3",
	&my_fops,
    .mode = S_IRWXUGO
};

static int put_block(uint64_t block, void* anything, uint64_t mountno) {
    loff_t position = (block * PAGE_CACHE_SIZE);
    kernel_write(mounts[mountno]->filp, anything, PAGE_CACHE_SIZE, &position);
    return position;
}

static void get_block(uint64_t block, void* anything, uint64_t mountno) {
    loff_t position = (block * PAGE_CACHE_SIZE);
    kernel_read(mounts[mountno]->filp, anything, PAGE_CACHE_SIZE, &position);
}

static int find_empty_key(struct key_block *curr_keys) {
    int i = 0;
    for(i = 0; i < 56; i++) {
        if(curr_keys -> entry[i].key[0] == 0) {
            return i;
        }
    }
    return -1;
}

static int find_key(struct key_block *curr_keys, int *blockno, int *blocksize, char *key_str) {
    int i = 0;
    for(i = 0; i < 56; i++) {
        printk(KERN_INFO "entry %d: key: %s\n", i, curr_keys->entry[i].key);
        if(!strcmp(curr_keys -> entry[i].key, key_str)) {
            *blockno = curr_keys->entry[i].data_blockno;
            *blocksize = curr_keys->entry[i].data_len;
            return 0;
        }
    }
    return -1;
}


// NEED PARAM, ARE WE RESERVING SUPER OR DATA OR FREE? data is 35 onward+

static uint64_t reserve_block(uint64_t len, uint64_t mountno) {
    // BUG_ON(len != 1);
    // search the free bitmap, find a 1, change 1 to 0 and return corresponding number that goes with the block.
   // loff_t size_in_bytes = i_size_read(file_inode(mounts[mountno]->filp));
    loff_t num_blocks = mounts[mountno]->super->fs_size/PAGE_CACHE_SIZE; //+ 1; // WHAT IF SIZE IN BYTES < PAGE SIZE?
    int i;
    printk(KERN_INFO "RESERVING 1 of %d BLOCKS\n", num_blocks);
    for(i = 0; i < (num_blocks+sizeof(unsigned long)-1)/sizeof(unsigned long); i++) {
        printk(KERN_INFO "bit %08x \n", mounts[mountno]->bitmap[i]);
    }
    uint64_t bit, prev = 0, count = 0;
   // uint64_t block = find_first_bit(mounts[mountno]->bitmap, num_blocks);
    uint64_t block = bitmap_find_free_region(mounts[mountno]->bitmap, num_blocks, 0);
    // we set all bits in the bitmap (1) and then clear them when in use (0)
    // here we then want to get the first set bit
    // for some reason find_first_bit always reports 0


    /* for_each_set_bit (bit, mounts[mountno]->bitmap, 1) {
        printk(KERN_INFO "%d bit %d prev", bit, prev);
        if (bit != 1) // if prev is not the same as our bit
        // in our case, this should be if prev != 1
            count = 0; // reset count
        prev = bit; // prev = our current bit
        // leaving this for now, this lets us clear multiple bits at once but for now just one.
        if (count == len) { // if our count+1 is the len we are looking for, return that bit
            printk(KERN_INFO "RETURNING BIT");
            bitmap_clear(mounts[mountno]->bitmap, bit, len);
            return bit;
        }
        count++;
    } */
    // printk(KERN_INFO "FAILED TO RESERVE BLOCK");
    printk(KERN_INFO "BLOCK TO RESERVE %d", block);
    //bitmap_clear(mounts[mountno]->bitmap, block, len);
    return block;
}

static int unreserve_block(uint64_t blockno, uint64_t mountno) {
    // modify bitmap to put 1 in the place of this block
    bitmap_clear(mounts[mountno]->bitmap, blockno, 1);
    return 0;
}

static void* make_block(void) {
    return kzalloc(PAGE_CACHE_SIZE, GFP_USER);
}

static void write_fs(char *key, char *data, uint64_t key_len, uint64_t data_len, uint64_t mountno) {
    if (mountno < num_mounts && mounts[mountno] != NULL) {
                struct key_block *curr_keys = make_block();
                get_block(mounts[mountno]->super->master, curr_keys, mountno);
                int i;
                i = find_empty_key(curr_keys);
                if(i < 0) {
                    // make a new key block
                   struct key_block *extra_key_block = make_block();
                   extra_key_block -> older_blockno = mounts[mountno]->super->master;
                    i = 0;
                    curr_keys = extra_key_block;
                } else {
                    unreserve_block(mounts[mountno]->super->master, mountno); // rewrite
                }

                if(data_len > PAGE_CACHE_SIZE - 8) {
                    // TODO
                }
                curr_keys -> entry[i].data_len = data_len;
                curr_keys -> entry[i].data_blockno = reserve_block(1, mountno);
                strcpy(curr_keys -> entry[i].key, key);

               // struct key_block new_key_block;
                uint64_t new_key_block;
                new_key_block = reserve_block(1, mountno);

                struct data_block *new_data_block;
                new_data_block = make_block();

                // fails here

                memcpy(new_data_block -> value, data, data_len);

                // make the proper amount of new data blocks if data > 8000

                new_data_block -> next_blockno = 0;

                put_block(curr_keys -> entry[i].data_blockno, new_data_block, mountno);
                put_block(new_key_block, curr_keys, mountno); // maybe only do this when full. keep in mem until some quota

                // this logic is wrong
                // is this right?
                mounts[mountno]->super->master = new_key_block;
    }
}

static int read_fs(struct ioctl_data *data, char *key, uint64_t mountno) {
    if (mounts[mountno]->filp) {
                struct key_block *curr_keys = make_block();
                int i = 0;
                int master_block;
                master_block = mounts[mountno]->super->master;
                printk(KERN_INFO "super master %d\n", mounts[mountno]->super->master);
                for(i = 0; i < mounts[mountno]->super->master_list_len; i++) {
                        get_block(master_block, curr_keys, mountno);
                        printk(KERN_INFO "got the block %d \n", master_block);
                        int blockno = 0;
                        //char *value = "";
                        int amnt_to_cpy = 0;
                        if(find_key(curr_keys, &blockno, &amnt_to_cpy, key) >= 0) {
                            int pages = 0;
                            printk(KERN_INFO "found matching key in %d amount %d", blockno, amnt_to_cpy);
                            if (data->value_len < amnt_to_cpy) {
                                amnt_to_cpy = data->value_len;
                            }
                            while(amnt_to_cpy > 0) {
                                struct data_block *our_data_block;
                                our_data_block = kzalloc(PAGE_CACHE_SIZE, GFP_USER);
                                get_block(blockno, our_data_block, mountno);
                                // we have the data block, need to read it and its following blocks
                                copy_to_user(data->value+pages*PAGE_CACHE_SIZE, our_data_block->value, min(PAGE_CACHE_SIZE, amnt_to_cpy));
                                amnt_to_cpy -= PAGE_CACHE_SIZE;
                                pages++;
                            } // last block might not be ma full 8192, error check for buffer size
                                return 0;
                        }
                        master_block = curr_keys -> older_blockno;
                }
    }
    return -EBADF;
}



static int my_ioctl(struct file *file, unsigned int unused, unsigned long arg)
{
    struct ioctl_data data;
    struct mount_data new_mount;
    printk(KERN_INFO "ioctl\n");
    int ret;
    if (copy_from_user(&data, (void __user*)arg, sizeof(struct ioctl_data))) {
        return -EFAULT;
    }
    printk(KERN_INFO "%d cmd\n", data.cmd);
    switch(data.cmd) {
        case 1:
            {
                printk(KERN_INFO "recieved.\n");
                char* key;
                printk(KERN_INFO "%d\n", data.key_len);
                printk(KERN_INFO "%d\n", data.value_len);
                if(data.key_len > 1000) {
                    return -1;
                }
                key = kzalloc(data.key_len+1, GFP_USER);
                char* value = kzalloc(data.value_len, GFP_USER);
                if (copy_from_user(key, data.key, data.key_len)) {
                    return -1;
                }
                if (copy_from_user(value, data.value, data.value_len)) {
                    return -1;
                }
                key[data.key_len] = 0;
                value[data.value_len] = 0;
                printk(KERN_INFO "%s %s %d %d\n", key, value, data.key_len, data.value_len);

            // int place = write_array(key, value, data.key_len, data.value_len);
            // if (place < 0) {
                //   return -EFAULT;
                //}
                // flip around logic
            //  write_pos = end_pos;
                //leftover = 0;
                //if (place == tracker) {
                //  tracker = tracker + 1;
                //} else {
                //  if (filp) {
                    //    ret = look_for_blanks(place, &data);
                    //  if (!ret) {
                        //    return -EFAULT;
                        //}
                // }
                //}
                write_fs(key, value, data.key_len, data.value_len, data.mountno);
                // use filp
                //char data[data.key_len+data.value_len+1] = key + " " + value;
                
                break;
            }
        case 2:
        {
            printk(KERN_INFO "%d\n", data.key_len);
            if(data.key_len > 1000) {
                return 0;
            }
            char* key;
            key = kzalloc(data.key_len+1, GFP_USER);
            if (copy_from_user(key, data.key, data.key_len)) {
                return -EFAULT;
            }
            key[data.key_len] = 0;
          //  printk(KERN_INFO "%s %d\n", key, tracker);
            // we only need to get the key here now go look for the value
            //read_array(&data, key);
            ret = read_fs(&data, key, data.mountno);
            if (ret < 0) {
                return ret;
            }
           // void *data_addr = data;
            if (copy_to_user((void __user*)arg, &data, sizeof(struct ioctl_data))) {
                return -EFAULT;
            }
            break;
        }
        case 3:
        {
            printk(KERN_INFO "%d\n", data.key_len);
            if(data.key_len > 1000) {
                return 0;
            }
            char* key;
            key = kzalloc(data.key_len+1, GFP_USER);
            if (copy_from_user(key, data.key, data.key_len)) {
                return -EFAULT;
            }
            key[data.key_len] = 0;
            //     printk(KERN_INFO "%s %d\n", key, tracker);
            int loop = 0;
            // we only need to get the key here now go look for the value
          //  for(loop = 0; loop < tracker; loop++) {
            //    printk(KERN_INFO "key %d of %d key %s\n", loop, tracker, all_data[loop].key);
              //  if (all_data[loop].key == NULL) {
                //    continue;
                //}
                //if(strcmp(all_data[loop].key, key) == 0) {
                  //  printk(KERN_INFO "found key... deleting\n");
                    //all_data[loop].value = NULL;
                    //all_data[loop].key = NULL;
                    //break;
                //}
            //}

            ssize_t ret;
            uint64_t key_len;
            uint64_t value_len;
            loff_t readPos = 0;
             /*     while(readPos < end_pos) {
                printk(KERN_INFO "%d\n", (int)readPos);
                ret = kernel_read(filp, &key_len, 8, &readPos);
                ret = kernel_read(filp, &value_len, 8, &readPos);
                char *curr_key = kzalloc(key_len+1, GFP_USER);
                void *curr_value = kzalloc(value_len, GFP_USER);
                printk(KERN_INFO "lengths %llu %llu\n", key_len, value_len);
                ret = kernel_read(filp, curr_key, key_len, &readPos);
                curr_key[key_len] = 0;
                if ((strcmp(curr_key, key) != 0)) {
                    readPos = readPos + value_len;
                    continue;
                } 
                int size = key_len + value_len;
                char* appendFile = kzalloc(size, GFP_USER);
                memset(appendFile, 0, size);  
                //appendFile[0] = 0;
                //qappendFile[size-1] = 0;
                void* key_mem_addr = NULL;
                key_mem_addr = appendFile;
                readPos -= key_len;
                kernel_write(filp, key_mem_addr, size, &readPos);
                return 0;
            }*/
        // implement get
        // add duplicate checking

        // need to free data here!!!!
        // use access_ok

        // use cmd to dilineate btwn put/get using cmd #
        }
        case 4: // new number for format from mk2efs.ls3:
        {
            // zero "file"   
            loff_t pos = 0;
            printk(KERN_INFO "%s\n device", data.value); 
	   //new_mount = kzalloc(sizeof(struct mount_data), GFP_USER);
            new_mount.filp = filp_open(data.value, O_RDWR, 0644);
            loff_t size_in_bytes = i_size_read(file_inode(new_mount.filp));
            loff_t num_blocks = size_in_bytes/PAGE_CACHE_SIZE; //+ 1; // WHAT IF SIZE IN BYTES < PAGE SIZE?
            loff_t num_freemap_bytes = (num_blocks + 7) / 8;
            loff_t num_freemap_blocks = (num_freemap_bytes + 8191) / 8192;
            loff_t num_total_freemap_blocks = num_freemap_blocks * 2;
            loff_t key_master_init = 32 + num_total_freemap_blocks;
            printk(KERN_INFO "formatting\n");
            while(pos < size_in_bytes) {
                void* ptr = NULL;
                ptr = "\0";
                ret = kernel_write(new_mount.filp, ptr, sizeof("\0"), &pos);
                pos = pos + sizeof("\0");
                if (ret == -1) break;
            }

            // write superblock, and its data
            new_mount.super = kzalloc(8192, GFP_USER);
            new_mount.super->fs_size = size_in_bytes;
            new_mount.super->magic = LS3_MAGIC;
            new_mount.super->master = key_master_init;
            new_mount.super->master_list_len = 1;
            new_mount.super->timestamp = ktime_get_real_ns();
            int i = 0;
            for(i = 0; i < (num_total_freemap_blocks); i++) {
                new_mount.super->used_bitmaps[i] = -1;
            }
            for(i = 0; i < (num_freemap_blocks); i++) {
                new_mount.super->used_bitmaps[i] = 32+i; // use half of the doubled superblocks
            }

            // BITMAP NOT BEING WRITTEN OR READ CORRECTLY

            //unsigned long *new_bitmap;
            //loff_t num_freemap_bytes = (total_blocks + 7) / 8;
            //loff_t num_freemap_blocks = (num_freemap_bytes + 8191) / 8192;
            uint64_t num_longs = ((num_blocks + (sizeof(unsigned long) * 8) - 1)) / (sizeof(unsigned long) * 8);
            uint64_t size_of_bitmap = num_longs * sizeof(unsigned long);
            new_mount.bitmap = kzalloc(round_to_multiple(size_of_bitmap, PAGE_CACHE_SIZE), GFP_USER);
            //new_mount.bitmap = kzalloc((num_freemap_blocks * 8192)/sizeof(unsigned long), GFP_USER);
            bitmap_clear(new_mount.bitmap, 0, num_blocks);

            printk(KERN_INFO "FORMAT mounted FS: fs_size: %d, total blocks: %d number of freemap_bytes: %llu, number fo freemap blocks: %llu, size of bitmap: %d\n", 
            size_in_bytes, num_blocks, num_freemap_bytes, num_freemap_blocks, size_of_bitmap);
            printk(KERN_INFO "bitmap first block: %d", new_mount.super->used_bitmaps[0]);

            //printk(KERN_INFO)

            // TODO, file is garbage
            bitmap_set(new_mount.bitmap, 0, 1);
            bitmap_set(new_mount.bitmap, 32, num_freemap_blocks);
            bitmap_set(new_mount.bitmap, key_master_init, 1);
 	        void* super_addr = NULL;
            super_addr = new_mount.super;
	        void* bitmap_addr = NULL;
	        bitmap_addr = new_mount.bitmap;
            pos = 0;
            kernel_write(new_mount.filp, super_addr, PAGE_CACHE_SIZE, &pos);
            pos = 32 * PAGE_CACHE_SIZE;
            //put_block(32, bitmap_addr, )
            kernel_write(new_mount.filp, bitmap_addr, num_freemap_blocks * PAGE_CACHE_SIZE, &pos);




           /* while(i < 31*PAGE_CACHE_SIZE) {
                ret = kernel_write(filp, "\0", sizeof("\0"), j);
                j = j + sizeof("0");
            }
            
            // write bitmap 1s
            // do I need to write a blank super and a blank bitmap to the file
            // bitmap is 0 - (block_size / fs-size) * 2
            int j = i;
            while(j < (i + (pos/PAGE_CACHE_SIZE)*2)) {
                ret = kernel_write(filp, "0", sizeof("0"), j);
                j = j + sizeof("0");
            }*/

            // kernel_write(filp, bitmap, sizeof(bitmap), 0);
           // kernel_write(filp, our_super, sizeof(our_super), 0)
            // is this at position 0?
            // kwrite
        }
    }
    return 0;
}

//populate data struct for file operations
static const struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open	= my_open,
	//.read 	= lfs_read_file,
	//.write  = lfs_write_file,
   // .open = &my_open,
	.release = &my_release,
	.unlocked_ioctl = (void*)&my_ioctl,
	.compat_ioctl = (void*)&my_ioctl,
};


static int __init main(void) {
    int retval;
    printk(KERN_INFO "Hello world!\n");
	register_filesystem(&ls3_type);
    //end_pos = scan_for_end();
    retval = misc_register(&my_device);
	return retval;
    //return register_filesystem(&lfs_type);
}

static void __exit cleanup(void)
{
   int i;
   for(i = num_mounts-1; i >= 0; i--) {
        if(mounts[i] != NULL) {
            printk(KERN_INFO "closing filp %p", mounts[i]->filp);
            filp_close(mounts[i]->filp, NULL);
        }
        // FREE ALL DATA HERE, UNMOUNT
   }

    misc_deregister(&my_device);
    unregister_filesystem(&ls3_type);
    printk(KERN_INFO "Cleaning up module.\n");
}

// need to get user space -> kernel space pointer for laat ioctl arg
// safeusercopy, usercopy, kernelcopy
module_init(main)
module_exit(cleanup)
