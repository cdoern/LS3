#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x32e21920, "module_layout" },
	{ 0xf0dd74e8, "simple_statfs" },
	{ 0xd09cd801, "generic_delete_inode" },
	{ 0x21ec6a82, "unregister_filesystem" },
	{ 0x7b52b91f, "misc_deregister" },
	{ 0x5e60f2f1, "misc_register" },
	{ 0x7ef641cb, "register_filesystem" },
	{ 0x23e6e058, "iput" },
	{ 0x77377edd, "d_make_root" },
	{ 0x2c7c4ad8, "simple_dir_operations" },
	{ 0xcfd32e98, "simple_dir_inode_operations" },
	{ 0x32742b40, "new_inode" },
	{ 0xf35141b2, "kmem_cache_alloc_trace" },
	{ 0x26087692, "kmalloc_caches" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x615911d7, "__bitmap_set" },
	{ 0x922f45a6, "__bitmap_clear" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0x9fb74541, "kernel_write" },
	{ 0xa0c4c91b, "filp_open" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xa374213c, "kernel_read" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x63a7c28c, "bitmap_find_free_region" },
	{ 0x801b1225, "mount_bdev" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x7ab3fff3, "kill_block_super" },
	{ 0x9e2fb826, "filp_close" },
	{ 0x92997ed8, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "0A76A3F4450A839FDAACC2C");
