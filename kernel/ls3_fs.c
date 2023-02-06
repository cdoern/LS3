
/*
static struct super_operations lfs_s_ops = {
    .statfs         = simple_statfs,
    .drop_inode     = generic_delete_inode,
};

static struct file_system_type lfs_type = {
	.owner 	= THIS_MODULE,
	.name	= "ls3fs",
	.get_sb	= lfs_get_super,
	.kill_sb = kill_litter_super,
    };



static int lfs_open(struct inode *inode, struct file *filp)
{
	    filp->private_data = inode->u.generic_ip;
	    return 0;
}

static ssize_t lfs_read_file(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	char *pdata = (char *) file->private_data;
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
	return size;
}

static ssize_t lfs_write_file(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	char *pdata = (char *) filp->private_data;
	char tmp[TMPSIZE];
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, data, size))
		return -EFAULT;
    pdata = tmp
	return count;
}

static struct inode *lfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_mode = mode;
		ret->i_uid = ret->i_gid = 0;
		ret->i_blksize = PAGE_CACHE_SIZE;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;
	}
	return ret;
}


static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name,
		char *data)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);

	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = lfs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &my_fops;
	inode->u.generic_ip = data;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}

static atomic_t data, subcounter;

static void lfs_create_files (struct super_block *sb, struct dentry *root)
{
	struct dentry *subdir;

    char *testing = "hello world"
	lfs_create_file(sb, root, "testing_file", &testing);
}

static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;

	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;

	root = lfs_make_inode (sb, S_IFDIR | 0755);
	if (! root)
		goto out;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_alloc_root(root);
	if (! root_dentry)
		goto out_iput;
	sb->s_root = root_dentry;

	lfs_create_files (sb, root_dentry);
	return 0;
	
  out_iput:
	iput(root);
  out:
	return -ENOMEM;
}

static struct super_block *lfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
    // calls our custom lfs_fill_super
	return get_sb_single(fst, flags, data, lfs_fill_super);
}
*/
