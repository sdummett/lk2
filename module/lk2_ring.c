// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/string.h>

#include "lk2.h"

void lk2_ring_init(struct lk2_ring *r)
{
	memset(r, 0, sizeof(*r));
	spin_lock_init(&r->lock);
}

void lk2_ring_push(struct lk2_ring *r, const struct lk2_entry *e)
{
	unsigned long flags;

	spin_lock_irqsave(&r->lock, flags);

	r->buf[r->head] = *e;
	r->head = (r->head + 1) % LK2_RING_SIZE;
	if (r->count < LK2_RING_SIZE)
		r->count++;

	spin_unlock_irqrestore(&r->lock, flags);
}

u32 lk2_ring_count(struct lk2_ring *r)
{
	unsigned long flags;
	u32 c;

	spin_lock_irqsave(&r->lock, flags);
	c = r->count;
	spin_unlock_irqrestore(&r->lock, flags);

	return c;
}

u32 lk2_ring_snapshot(struct lk2_ring *r, struct lk2_entry *dst, u32 max)
{
	unsigned long flags;
	u32 c, i;
	u32 start;

	spin_lock_irqsave(&r->lock, flags);

	c = r->count;
	if (c > max)
		c = max;

	// Chronological order: oldest -> newest.
	// Oldest index is (head - count) modulo size.
	start = (r->head + LK2_RING_SIZE - r->count) % LK2_RING_SIZE;

	for (i = 0; i < c; i++)
	{
		u32 idx = (start + i) % LK2_RING_SIZE;
		dst[i] = r->buf[idx];
	}

	spin_unlock_irqrestore(&r->lock, flags);

	return c;
}
