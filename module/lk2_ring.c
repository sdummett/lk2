// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "lk2.h"

int lk2_ring_init(struct lk2_ring *r)
{
	memset(r, 0, sizeof(*r));
	spin_lock_init(&r->lock);

	r->buf = kvmalloc_array(LK2_RING_INIT_CAP, sizeof(*r->buf), GFP_KERNEL);
	if (!r->buf)
		return -ENOMEM;

	r->capacity = LK2_RING_INIT_CAP;
	r->count = 0;
	return 0;
}

void lk2_ring_destroy(struct lk2_ring *r)
{
	kvfree(r->buf);
	r->buf = NULL;
	r->capacity = 0;
	r->count = 0;
}

void lk2_ring_push(struct lk2_ring *r, const struct lk2_entry *e)
{
	unsigned long flags;
	struct lk2_entry *new_buf;
	u32 new_cap;

	spin_lock_irqsave(&r->lock, flags);

	if (r->count >= r->capacity)
	{
		new_cap = r->capacity * 2;
		spin_unlock_irqrestore(&r->lock, flags);

		new_buf = kvmalloc_array(new_cap, sizeof(*new_buf), GFP_KERNEL);
		if (!new_buf)
		{
			pr_warn_ratelimited("lk2: failed to grow buffer, dropping event\n");
			return;
		}

		spin_lock_irqsave(&r->lock, flags);
		if (r->count < r->capacity)
		{
			spin_unlock_irqrestore(&r->lock, flags);
			kvfree(new_buf);
			spin_lock_irqsave(&r->lock, flags);
		}
		else
		{
			memcpy(new_buf, r->buf, r->count * sizeof(*r->buf));
			kvfree(r->buf);
			r->buf = new_buf;
			r->capacity = new_cap;
		}
	}

	r->buf[r->count++] = *e;

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
	u32 c;

	spin_lock_irqsave(&r->lock, flags);

	c = r->count;
	if (c > max)
		c = max;

	memcpy(dst, r->buf, c * sizeof(*dst));

	spin_unlock_irqrestore(&r->lock, flags);

	return c;
}
