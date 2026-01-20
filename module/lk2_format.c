// SPDX-License-Identifier: GPL-2.0

#include "lk2.h"

// Key name table (English)
// This is intentionally limited to common keys;, unknown codes return "Unknown"
// You can extend this table without touching the rest of the driver
static const char *const lk2_key_names[KEY_MAX + 1] = {
	[KEY_ESC] = "Escape",
	[KEY_1] = "1",
	[KEY_2] = "2",
	[KEY_3] = "3",
	[KEY_4] = "4",
	[KEY_5] = "5",
	[KEY_6] = "6",
	[KEY_7] = "7",
	[KEY_8] = "8",
	[KEY_9] = "9",
	[KEY_0] = "0",
	[KEY_MINUS] = "Minus",
	[KEY_EQUAL] = "Equal",
	[KEY_BACKSPACE] = "Backspace",
	[KEY_TAB] = "Tab",
	[KEY_Q] = "Q",
	[KEY_W] = "W",
	[KEY_E] = "E",
	[KEY_R] = "R",
	[KEY_T] = "T",
	[KEY_Y] = "Y",
	[KEY_U] = "U",
	[KEY_I] = "I",
	[KEY_O] = "O",
	[KEY_P] = "P",
	[KEY_LEFTBRACE] = "LeftBrace",
	[KEY_RIGHTBRACE] = "RightBrace",
	[KEY_ENTER] = "Return",
	[KEY_LEFTCTRL] = "LeftCtrl",
	[KEY_A] = "A",
	[KEY_S] = "S",
	[KEY_D] = "D",
	[KEY_F] = "F",
	[KEY_G] = "G",
	[KEY_H] = "H",
	[KEY_J] = "J",
	[KEY_K] = "K",
	[KEY_L] = "L",
	[KEY_SEMICOLON] = "Semicolon",
	[KEY_APOSTROPHE] = "Apostrophe",
	[KEY_GRAVE] = "Grave",
	[KEY_LEFTSHIFT] = "LeftShift",
	[KEY_BACKSLASH] = "Backslash",
	[KEY_Z] = "Z",
	[KEY_X] = "X",
	[KEY_C] = "C",
	[KEY_V] = "V",
	[KEY_B] = "B",
	[KEY_N] = "N",
	[KEY_M] = "M",
	[KEY_COMMA] = "Comma",
	[KEY_DOT] = "Dot",
	[KEY_SLASH] = "Slash",
	[KEY_RIGHTSHIFT] = "RightShift",
	[KEY_KPASTERISK] = "KPAsterisk",
	[KEY_LEFTALT] = "LeftAlt",
	[KEY_SPACE] = "Space",
	[KEY_CAPSLOCK] = "CapsLock",
	[KEY_F1] = "F1",
	[KEY_F2] = "F2",
	[KEY_F3] = "F3",
	[KEY_F4] = "F4",
	[KEY_F5] = "F5",
	[KEY_F6] = "F6",
	[KEY_F7] = "F7",
	[KEY_F8] = "F8",
	[KEY_F9] = "F9",
	[KEY_F10] = "F10",
	[KEY_NUMLOCK] = "NumLock",
	[KEY_SCROLLLOCK] = "ScrollLock",
	[KEY_F11] = "F11",
	[KEY_F12] = "F12",
	[KEY_RIGHTCTRL] = "RightCtrl",
	[KEY_RIGHTALT] = "RightAlt",
	[KEY_HOME] = "Home",
	[KEY_UP] = "Up",
	[KEY_PAGEUP] = "PageUp",
	[KEY_LEFT] = "Left",
	[KEY_RIGHT] = "Right",
	[KEY_END] = "End",
	[KEY_DOWN] = "Down",
	[KEY_PAGEDOWN] = "PageDown",
	[KEY_INSERT] = "Insert",
	[KEY_DELETE] = "Delete",
};

const char *lk2_key_name(u16 keycode)
{
	if (keycode <= KEY_MAX && lk2_key_names[keycode])
		return lk2_key_names[keycode];
	return "Unknown";
}

/*
 * ASCII conversion (layout-dependent)
 * This mapping assumes a US QWERTY keyboard
 */
static const char lk2_ascii_noshift[KEY_MAX + 1] = {
	[KEY_A] = 'a',
	[KEY_B] = 'b',
	[KEY_C] = 'c',
	[KEY_D] = 'd',
	[KEY_E] = 'e',
	[KEY_F] = 'f',
	[KEY_G] = 'g',
	[KEY_H] = 'h',
	[KEY_I] = 'i',
	[KEY_J] = 'j',
	[KEY_K] = 'k',
	[KEY_L] = 'l',
	[KEY_M] = 'm',
	[KEY_N] = 'n',
	[KEY_O] = 'o',
	[KEY_P] = 'p',
	[KEY_Q] = 'q',
	[KEY_R] = 'r',
	[KEY_S] = 's',
	[KEY_T] = 't',
	[KEY_U] = 'u',
	[KEY_V] = 'v',
	[KEY_W] = 'w',
	[KEY_X] = 'x',
	[KEY_Y] = 'y',
	[KEY_Z] = 'z',

	[KEY_1] = '1',
	[KEY_2] = '2',
	[KEY_3] = '3',
	[KEY_4] = '4',
	[KEY_5] = '5',
	[KEY_6] = '6',
	[KEY_7] = '7',
	[KEY_8] = '8',
	[KEY_9] = '9',
	[KEY_0] = '0',

	[KEY_SPACE] = ' ',
	[KEY_MINUS] = '-',
	[KEY_EQUAL] = '=',
	[KEY_LEFTBRACE] = '[',
	[KEY_RIGHTBRACE] = ']',
	[KEY_BACKSLASH] = '\\',
	[KEY_SEMICOLON] = ';',
	[KEY_APOSTROPHE] = '\'',
	[KEY_GRAVE] = '`',
	[KEY_COMMA] = ',',
	[KEY_DOT] = '.',
	[KEY_SLASH] = '/',
	[KEY_TAB] = '\t',
	[KEY_ENTER] = '\n',
};

static const char lk2_ascii_shift[KEY_MAX + 1] = {
	[KEY_A] = 'A',
	[KEY_B] = 'B',
	[KEY_C] = 'C',
	[KEY_D] = 'D',
	[KEY_E] = 'E',
	[KEY_F] = 'F',
	[KEY_G] = 'G',
	[KEY_H] = 'H',
	[KEY_I] = 'I',
	[KEY_J] = 'J',
	[KEY_K] = 'K',
	[KEY_L] = 'L',
	[KEY_M] = 'M',
	[KEY_N] = 'N',
	[KEY_O] = 'O',
	[KEY_P] = 'P',
	[KEY_Q] = 'Q',
	[KEY_R] = 'R',
	[KEY_S] = 'S',
	[KEY_T] = 'T',
	[KEY_U] = 'U',
	[KEY_V] = 'V',
	[KEY_W] = 'W',
	[KEY_X] = 'X',
	[KEY_Y] = 'Y',
	[KEY_Z] = 'Z',

	[KEY_1] = '!',
	[KEY_2] = '@',
	[KEY_3] = '#',
	[KEY_4] = '$',
	[KEY_5] = '%',
	[KEY_6] = '^',
	[KEY_7] = '&',
	[KEY_8] = '*',
	[KEY_9] = '(',
	[KEY_0] = ')',

	[KEY_SPACE] = ' ',
	[KEY_MINUS] = '_',
	[KEY_EQUAL] = '+',
	[KEY_LEFTBRACE] = '{',
	[KEY_RIGHTBRACE] = '}',
	[KEY_BACKSLASH] = '|',
	[KEY_SEMICOLON] = ':',
	[KEY_APOSTROPHE] = '"',
	[KEY_GRAVE] = '~',
	[KEY_COMMA] = '<',
	[KEY_DOT] = '>',
	[KEY_SLASH] = '?',
	[KEY_TAB] = '\t',
	[KEY_ENTER] = '\n',
};

char lk2_keycode_to_ascii(u16 keycode, bool shift_down)
{
	if (keycode > KEY_MAX)
		return 0;

	return shift_down ? lk2_ascii_shift[keycode] : lk2_ascii_noshift[keycode];
}

size_t lk2_format_line(const struct lk2_entry *e, char *dst, size_t dst_len)
{
	const char *name = lk2_key_name(e->keycode);
	const char *state = (e->state == LK2_PRESSED) ? "Pressed" : "Released";

	// Mandatory line format:
	// HH:MM:SS Name of the key(key code) Pressed / Released\n
	return scnprintf(dst, dst_len, "%02u:%02u:%02u %s(%u) %s\n",
					 e->t.hour, e->t.min, e->t.sec,
					 name, e->keycode, state);
}
