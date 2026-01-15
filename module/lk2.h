// SPDX-License-Identifier: GPL-2.0
#ifndef LK2_H
#define LK2_H

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/miscdevice.h>

// ----- lk2_main.c ----- //

// Device node name created by lk2_main.c via miscdevice
#define LK2_DEVICE_NAME "keylogs"

struct lk2_file_ctx
{
	char *buf;
	size_t len;
};

#endif /* LK2_H */
