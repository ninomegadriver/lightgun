/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2005 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

#include "portable.h"

#include "jevent.h"
#include "log.h"
#include "oslinux.h"
#include "error.h"
#include "event.h"
#include "snstring.h"

#include <linux/input.h>

#ifndef BTN_PLAY
#define BTN_PLAY 0x13f /* missing in kernel 4.19 but used by joystick */
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/***************************************************************************/
/* Acts Labs Lightgun */

/**
 * Define to use the Acts Labs Lightgun Reload Hack.
 * For the ACT LABS guns, shooting offscreen, or pressing the side button,
 * causes a BTN_RIGHT event code. Shooting directly at the screen produces
 * an X and Y axis event, aswell as a BTN_LEFT event. For the second button
 * BTN_RIGHT, I set the abs values to offscreen and produce a normal first button
 * event BTN_LEFT.
 */
#define USE_ACTLABS_HACK

#ifdef USE_ACTLABS_HACK
#define ACTLABS_VENDOR 0x061c
#define ACTLABS_DEVICE_1 0xa800
#define ACTLABS_DEVICE_2 0xa700
#define ACTLABS_BUTTON BTN_RIGHT
#define ACTLABS_FRAME_DELAY 2
#endif

/***************************************************************************/
/* Event */

#define EVENT_JOYSTICK_MAX 8
#define EVENT_JOYSTICK_DEVICE_MAX 32
#define EVENT_JOYSTICK_BUTTON_MAX 64
#define EVENT_JOYSTICK_STICK_MAX 16
#define EVENT_JOYSTICK_NAME_MAX 128
#define EVENT_JOYSTICK_AXE_MAX 8
#define EVENT_JOYSTICK_REL_MAX 8

struct joystick_button_context {
	unsigned code;
	int revbind;
	adv_bool state;
	char name[EVENT_JOYSTICK_NAME_MAX];
	int sort_order;
};

struct joystick_axe_context {
	int code;
	int value;
	int value_adj; /**< Value adjusted in range -JOYSTICK_DRIVER_BASE, JOYSTICK_DRIVER_BASE. */
	int min;
	int max;
	int fuzz;
	int flat;
	int digit_low;
	int digit_high;
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_stick_context {
	unsigned axe_mac;
	struct joystick_axe_context axe_map[EVENT_JOYSTICK_AXE_MAX];
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_rel_context {
	unsigned code;
	int value;
	char name[EVENT_JOYSTICK_NAME_MAX];
};

struct joystick_item_context {
	int f;
	char desc[DEVICE_NAME_MAX];
	unsigned vendor;
	unsigned product;
	unsigned version;
	unsigned bus;
#ifdef USE_ACTLABS_HACK
	adv_bool actlabs_hack_enable; /**< If the ACT Labs hack is enabled. */
	unsigned actlabs_hack_counter; /**< Poll counter at the hack start. */
#endif
	unsigned char evtype_bitmask[EV_MAX / 8 + 1];
	unsigned stick_mac;
	struct joystick_stick_context stick_map[EVENT_JOYSTICK_STICK_MAX];
	unsigned button_mac;
	struct joystick_button_context button_map[EVENT_JOYSTICK_BUTTON_MAX];
	unsigned rel_mac;
	struct joystick_rel_context rel_map[EVENT_JOYSTICK_REL_MAX];
	int bind_map[JOYB_MAX];
};

struct joystickb_event_context {
	unsigned counter; /**< Poll counter. Increased at every poll. */
	unsigned mac;
	struct joystick_item_context map[EVENT_JOYSTICK_MAX];
};

static struct joystickb_event_context event_state;

/***************************************************************************/

static adv_device DEVICE[] = {
	{ "auto", -1, "Linux input-event joystick" },
	{ 0, 0, 0 }
};

#define ABS_UNASSIGNED -1

static int button_compare(const void* void_a, const void* void_b)
{
	const struct joystick_button_context* a = void_a;
	const struct joystick_button_context* b = void_b;

	if (a->sort_order < b->sort_order)
		return -1;
	if (a->sort_order > b->sort_order)
		return 1;
	return 0;
}

static adv_error joystickb_setup(struct joystick_item_context* item, int f)
{
	unsigned char key_bitmask[KEY_MAX / 8 + 1];
	unsigned char abs_bitmask[ABS_MAX / 8 + 1];
	unsigned char rel_bitmask[REL_MAX / 8 + 1];
	unsigned i;
	unsigned short device_info[4];
	int override_begin;
	int override_end;

	struct button_entry {
		int code;
		const char* name;
		int bind;
	} button_map[] = {
#ifdef BTN_TRIGGER
		{ BTN_TRIGGER, "trigger" }, /* joystick */
#endif
#ifdef BTN_THUMB
		{ BTN_THUMB, "thumb" }, /* joystick */
#endif
#ifdef BTN_THUMB2
		{ BTN_THUMB2, "thumb2" }, /* joystick */
#endif
#ifdef BTN_TOP
		{ BTN_TOP, "top" }, /* joystick */
#endif
#ifdef BTN_TOP2
		{ BTN_TOP2, "top2" }, /* joystick */
#endif
#ifdef BTN_PINKIE
		{ BTN_PINKIE, "pinkie" }, /* joystick */
#endif
#ifdef BTN_BASE
		{ BTN_BASE, "base" }, /* joystick */
#endif
#ifdef BTN_BASE2
		{ BTN_BASE2, "base2" }, /* joystick */
#endif
#ifdef BTN_BASE3
		{ BTN_BASE3, "base3" }, /* joystick */
#endif
#ifdef BTN_BASE4
		{ BTN_BASE4, "base4" }, /* joystick */
#endif
#ifdef BTN_BASE5
		{ BTN_BASE5, "base5" }, /* joystick */
#endif
#ifdef BTN_BASE6
		{ BTN_BASE6, "base6" }, /* joystick */
#endif
		{ 0x12c, "extra1" }, /* extra button used incorrectly by some joysticks */
		{ 0x12d, "extra2" }, /* extra button used incorrectly by some joysticks */
		{ 0x12e, "extra3" }, /* extra button used incorrectly by some joysticks */
#ifdef BTN_DEAD
		{ BTN_DEAD, "dead" }, /* not really clear what is it */
#endif
		/* note that the happy name has no meaning */
		/* if not to make "happy" joysticks with many buttons */
#ifdef BTN_TRIGGER_HAPPY1
		{ BTN_TRIGGER_HAPPY1, "happy1" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY2
		{ BTN_TRIGGER_HAPPY2, "happy2" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY3
		{ BTN_TRIGGER_HAPPY3, "happy3" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY4
		{ BTN_TRIGGER_HAPPY4, "happy4" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY5
		{ BTN_TRIGGER_HAPPY5, "happy5" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY6
		{ BTN_TRIGGER_HAPPY6, "happy6" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY7
		{ BTN_TRIGGER_HAPPY7, "happy7" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY8
		{ BTN_TRIGGER_HAPPY8, "happy8" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY9
		{ BTN_TRIGGER_HAPPY9, "happy9" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY10
		{ BTN_TRIGGER_HAPPY10, "happy10" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY11
		{ BTN_TRIGGER_HAPPY11, "happy11" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY12
		{ BTN_TRIGGER_HAPPY12, "happy12" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY13
		{ BTN_TRIGGER_HAPPY13, "happy13" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY14
		{ BTN_TRIGGER_HAPPY14, "happy14" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY15
		{ BTN_TRIGGER_HAPPY15, "happy15" }, /* joystick */
#endif
#ifdef BTN_TRIGGER_HAPPY16
		{ BTN_TRIGGER_HAPPY16, "happy16" }, /* joystick */
#endif
#ifdef BTN_A
		{ BTN_A, "a", JOYB_A }, /* gamepad */
#endif
#ifdef BTN_B
		{ BTN_B, "b", JOYB_B }, /* gamepad */
#endif
#ifdef BTN_C
		{ BTN_C, "c", JOYB_C }, /* gamepad */
#endif
#ifdef BTN_X
		{ BTN_X, "x", JOYB_X }, /* gamepad */
#endif
#ifdef BTN_Y
		{ BTN_Y, "y", JOYB_Y }, /* gamepad */
#endif
#ifdef BTN_Z
		{ BTN_Z, "z", JOYB_Z }, /* gamepad */
#endif
#ifdef BTN_TL
		{ BTN_TL, "tl", JOYB_TL }, /* gamepad (top left) */
#endif
#ifdef BTN_TR
		{ BTN_TR, "tr", JOYB_TR }, /* gamepad (top right) */
#endif
#ifdef BTN_TL2
		{ BTN_TL2, "tl2", JOYB_TL2 }, /* gamepad (top left 2) */
#endif
#ifdef BTN_TR2
		{ BTN_TR2, "tr2", JOYB_TR2 }, /* gamepad (top right 2) */
#endif
#ifdef BTN_SELECT
		{ BTN_SELECT, "select", JOYB_SELECT }, /* gamepad */
#endif
#ifdef BTN_START
		{ BTN_START, "start", JOYB_START }, /* gamepad */
#endif
#ifdef BTN_MODE
		{ BTN_MODE, "mode", JOYB_MODE }, /* gamepad */
#endif
#ifdef BTN_THUMBL
		{ BTN_THUMBL, "thumbl", JOYB_THUMBL }, /* gamepad (thumb left) */
#endif
#ifdef BTN_THUMBR
		{ BTN_THUMBR, "thumbr", JOYB_THUMBR }, /* gamepad (thumb right) */
#endif
#ifdef BTN_PLAY
		{ BTN_PLAY, "play", JOYB_PLAY }, /* gamepad (play) */
#endif
#ifdef BTN_GEAR_DOWN
		{ BTN_GEAR_DOWN, "gear_down", JOYB_GEAR_DOWN }, /* wheel */
#endif
#ifdef BTN_GEAR_UP
		{ BTN_GEAR_UP, "gear_up", JOYB_GEAR_UP }, /* wheel */
#endif
#ifdef BTN_0
		{ BTN_0, "0" }, /* misc */
#endif
#ifdef BTN_1
		{ BTN_1, "1" }, /* misc */
#endif
#ifdef BTN_2
		{ BTN_2, "2" }, /* misc */
#endif
#ifdef BTN_3
		{ BTN_3, "3" }, /* misc */
#endif
#ifdef BTN_4
		{ BTN_4, "4" }, /* misc */
#endif
#ifdef BTN_5
		{ BTN_5, "5" }, /* misc */
#endif
#ifdef BTN_6
		{ BTN_6, "6" }, /* misc */
#endif
#ifdef BTN_7
		{ BTN_7, "7" }, /* misc */
#endif
#ifdef BTN_8
		{ BTN_8, "8" }, /* misc */
#endif
#ifdef BTN_9
		{ BTN_9, "9" }, /* misc */
#endif
#ifdef BTN_LEFT
		{ BTN_LEFT, "left" }, /* ball */
#endif
#ifdef BTN_RIGHT
		{ BTN_RIGHT, "right" }, /* ball */
#endif
#ifdef BTN_MIDDLE
		{ BTN_MIDDLE, "middle" }, /* ball/lightgun first button */
#endif
#ifdef BTN_SIDE
		{ BTN_SIDE, "side" }, /* ball/lightgun second button */
#endif
#ifdef BTN_EXTRA
		{ BTN_EXTRA, "extra" }, /* ball */
#endif
#ifdef BTN_FORWARD
		{ BTN_FORWARD, "forward" }, /* ball */
#endif
#ifdef BTN_BACK
		{ BTN_BACK, "back" } /* ball */
#endif
	};

	struct override_entry {
		unsigned vendor;
		unsigned product;
		const char* desc;
		unsigned index;
		const char* name;
		int bind;
	} override_map[] = {
		/* QMOX SNES */
		{ 0x0810, 0xe501, "usb gamepad           ", 1, "a", JOYB_A },
		{ 0x0810, 0xe501, "usb gamepad           ", 2, "b", JOYB_B },
		{ 0x0810, 0xe501, "usb gamepad           ", 0, "x", JOYB_X },
		{ 0x0810, 0xe501, "usb gamepad           ", 3, "y", JOYB_Y },
		{ 0x0810, 0xe501, "usb gamepad           ", 4, "tl", JOYB_TL },
		{ 0x0810, 0xe501, "usb gamepad           ", 6, "tr", JOYB_TR },
		{ 0x0810, 0xe501, "usb gamepad           ", 8, "select", JOYB_SELECT },
		{ 0x0810, 0xe501, "usb gamepad           ", 9, "start", JOYB_START },
		/* iNNEXT SNES */
		{ 0x0079, 0x0011, "USB Gamepad ", 1, "a", JOYB_A },
		{ 0x0079, 0x0011, "USB Gamepad ", 2, "b", JOYB_B },
		{ 0x0079, 0x0011, "USB Gamepad ", 0, "x", JOYB_X },
		{ 0x0079, 0x0011, "USB Gamepad ", 3, "y", JOYB_Y },
		{ 0x0079, 0x0011, "USB Gamepad ", 4, "tl", JOYB_TL },
		{ 0x0079, 0x0011, "USB Gamepad ", 5, "tr", JOYB_TR },
		{ 0x0079, 0x0011, "USB Gamepad ", 8, "select", JOYB_SELECT },
		{ 0x0079, 0x0011, "USB Gamepad ", 9, "start", JOYB_START },
		/* 8BITDO SF30 - ANDROID mode (START) */
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 0, "a", JOYB_A },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 1, "b", JOYB_B },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 3, "x", JOYB_X },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 4, "y", JOYB_Y },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 6, "tl", JOYB_TL },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 7, "tr", JOYB_TR },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 10, "select", JOYB_SELECT },
		{ 0x2dc8, 0x2830, "8Bitdo SFC30 GamePad", 11, "start", JOYB_START },
		/* 8BITDO SF30 - PC mode (START+X) */
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 1, "a", JOYB_A },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 0, "b", JOYB_B },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 3, "x", JOYB_X },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 2, "y", JOYB_Y },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 4, "tl", JOYB_TL },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 5, "tr", JOYB_TR },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 6, "select", JOYB_SELECT },
		{ 0x045e, 0x02e0, "8Bitdo SFC30 GamePad(x)", 7, "start", JOYB_START },
		/* 8BITDO SF30 - MAC mode (START+A) */
		/* ... listed as "Wireless Controller" as "input-gaming", but do not show in joystick */
		/* 8BITDO SF30 - SWITCH mode (START+Y) */
		/* 0x57e, 0x2007, ""Pro Controller" - No input is working (need firmare update ?) */
		/* 8Bitdo SF30 Pro - SWITCH mode (START+Y), 8Bitdo N30 Pro - SWITCH mode (POWER+Y) */
		{ 0x057e, 0x2009, "Pro Controller", 1, "a", JOYB_A },
		{ 0x057e, 0x2009, "Pro Controller", 0, "b", JOYB_B },
		{ 0x057e, 0x2009, "Pro Controller", 3, "x", JOYB_X },
		{ 0x057e, 0x2009, "Pro Controller", 2, "y", JOYB_Y },
		{ 0x057e, 0x2009, "Pro Controller", 4, "tl", JOYB_TL },
		{ 0x057e, 0x2009, "Pro Controller", 5, "tr", JOYB_TR },
		{ 0x057e, 0x2009, "Pro Controller", 6, "tl2", JOYB_TL2 },
		{ 0x057e, 0x2009, "Pro Controller", 7, "tr2", JOYB_TR2 },
		{ 0x057e, 0x2009, "Pro Controller", 8, "select", JOYB_SELECT },
		{ 0x057e, 0x2009, "Pro Controller", 9, "start", JOYB_START },
		/* 8Bitdo SF30 Pro - ANDROID (START+B) */
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 0, "a", JOYB_A },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 1, "b", JOYB_B },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 3, "x", JOYB_X },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 4, "y", JOYB_Y },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 6, "tl", JOYB_TL },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 7, "tr", JOYB_TR },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 8, "tl2", JOYB_TL2 },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 9, "tr2", JOYB_TR2 },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 10, "select", JOYB_SELECT },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 11, "start", JOYB_START },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 2, "mode", JOYB_MODE },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 13, "thumbl", JOYB_THUMBL },
		{ 0x2dc8, 0x6100, "8Bitdo SF30 Pro", 14, "thumbr", JOYB_THUMBR },
		/* 8Bitdo SF30 Pro - PC mode (START+Y) */
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 1, "a", JOYB_A },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 0, "b", JOYB_B },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 3, "x", JOYB_X },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 2, "y", JOYB_Y },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 4, "tl", JOYB_TL },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 5, "tr", JOYB_TR },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 6, "select", JOYB_SELECT },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 7, "start", JOYB_START },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 8, "thumbl", JOYB_THUMBL },
		{ 0x045e, 0x02e0, "8Bitdo SF30 Pro", 9, "thumbr", JOYB_THUMBR },
		/* 8Bitdo SF30 Pro - MAC mode (START+A) */
		/* ... listed as "Wireless Controller" as "input-gaming", but do not show in joystick */
		/* 8Bitdo N30 Pro - ANDROID mode (POWER) */
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 0, "a", JOYB_A },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 1, "b", JOYB_B },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 3, "x", JOYB_X },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 4, "y", JOYB_Y },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 6, "tl", JOYB_TL },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 7, "tr", JOYB_TR },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 8, "tl2", JOYB_TL2 },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 9, "tr2", JOYB_TR2 },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 10, "select", JOYB_SELECT },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 11, "start", JOYB_START },
		{ 0x2dc8, 0x3820, "8Bitdo NES30 Pro", 2, "mode", JOYB_MODE },
		/* 8Bitdo N30 Pro - PC mode (POWER+X) */
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 1, "a", JOYB_A },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 0, "b", JOYB_B },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 3, "x", JOYB_X },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 2, "y", JOYB_Y },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 4, "tl", JOYB_TL },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 5, "tr", JOYB_TR },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 6, "select", JOYB_SELECT },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 7, "start", JOYB_START },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 8, "thumbl", JOYB_THUMBL },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 9, "thumbr", JOYB_THUMBR },
		{ 0x045e, 0x02e0, "8Bitdo NES30 Pro(x)", 10, "mode", JOYB_MODE },
		/* 8Bitdo N30 Pro - MAC mode (POWER+A) */
		/* ... listed as "Wireless Controller" as "input-gaming", but do not show in joystick */

/*
 * Include libretro definitions from:
 * https://github.com/libretro/retroarch-joypad-autoconfig/udev
 * and converted with joverride.python.
 */
#include "joverride.dat"
	};

	/* WARNING: It must be syncronized with the list in event.c */
	struct stick_entry {
		struct axe_entry {
			int code;
			const char* name;
		} axe_map[7];
		const char* name;
	} stick_map[] = {
		{ { { ABS_X, "x" }, { ABS_Y, "y" }, { ABS_Z, "z" }, { ABS_RX, "rx" }, { ABS_RY, "ry" }, { ABS_RZ, "rz" }, { ABS_UNASSIGNED, 0 } }, "stick" },
#ifdef ABS_GAS
		{ { { ABS_GAS, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "gas" }, /* IT:acceleratore */
#endif
#ifdef ABS_BRAKE
		{ { { ABS_BRAKE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "brake" }, /* IT:freno */
#endif
#ifdef ABS_WHEEL
		{ { { ABS_WHEEL, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "wheel" }, /* IT:volante */
#endif
#ifdef ABS_HAT0X
		{ { { ABS_HAT0X, "x" }, { ABS_HAT0Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat" }, /* IT:mini joystick digitale */
#endif
#ifdef ABS_HAT1X
		{ { { ABS_HAT1X, "x" }, { ABS_HAT1Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat2" },
#endif
#ifdef ABS_HAT2X
		{ { { ABS_HAT2X, "x" }, { ABS_HAT2Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat3" },
#endif
#ifdef ABS_HAT3X
		{ { { ABS_HAT3X, "x" }, { ABS_HAT3Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "hat4" },
#endif
#ifdef ABS_THROTTLE
		{ { { ABS_THROTTLE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "throttle" },
#endif
#ifdef ABS_RUDDER
		{ { { ABS_RUDDER, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "rudder" }, /* IT:timone */
#endif
		/* { { { ABS_PRESSURE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "pressure" }, */ /* tablet */
		/* { { { ABS_DISTANCE, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "distance" }, */ /* tablet */
		/* { { { ABS_TILT_X, "x" }, { ABS_TILT_Y, "y" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "tilt" }, */ /* tablet */
		/* { { { ABS_VOLUME, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "volume" }, */ /* not an action control */
#ifdef ABS_MISC
		{ { { ABS_MISC, "mono" }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 }, { ABS_UNASSIGNED, 0 } }, "misc" }
#endif
	};

	struct rel_entry {
		int code;
		const char* name;
	} rel_map[] = {
#ifdef REL_X
		{ REL_X, "x" },
#endif
#ifdef REL_Y
		{ REL_Y, "y" },
#endif
#ifdef REL_Z
		{ REL_Z, "z" },
#endif
#ifdef REL_WHEEL
		{ REL_WHEEL, "wheel" }, /* (IT: rotella del mouse verticale) */
#endif
#ifdef REL_HWHEEL
		{ REL_HWHEEL, "hwheel" }, /* (IT: rotella del mouse orizzontale) */
#endif
#ifdef REL_DIAL
		{ REL_DIAL, "dial" }, /* (IT: manopola che gira) */
#endif
#ifdef REL_MISC
		{ REL_MISC, "misc" }
#endif
	};

	item->f = f;

#ifdef USE_ACTLABS_HACK
	item->actlabs_hack_enable = 0;
#endif

	memset(key_bitmask, 0, sizeof(key_bitmask));
	if (event_test_bit(EV_KEY, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_KEY, sizeof(key_bitmask)), key_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_KEY,%d))\n", (int)KEY_MAX));
			return -1;
		}
	}

	for (i = 0; i < JOYB_MAX; ++i)
		item->bind_map[i] = -1;

	/* check if there is an override */
	override_begin = 0;
	override_end = 0;
	for (i = 0; i < sizeof(override_map) / sizeof(override_map[0]); ++i) {
		if (override_map[i].vendor != item->vendor
			|| override_map[i].product != item->product
			|| strcmp(override_map[i].desc, item->desc) != 0
		) {
			if (override_begin != override_end)
				break;
		} else {
			if (override_begin == override_end)
				override_begin = i;
			override_end = i + 1;
		}
	}

	if (override_begin != override_end) {
		log_std(("event: override %04x:%04x '%s' %u %u\n", item->vendor, item->product, item->desc, override_begin, override_end));

		/*
		 * Iterate over all possible buttons
		 *
		 * Start from BTN_MISC like libretro to be compatible with its mapping:
		 * See: https://github.com/libretro/RetroArch/blob/master/input/drivers_joypad/udev_joypad.c
		 */
		unsigned index = 0;
		item->button_mac = 0;
		for (i = BTN_MISC; i < KEY_MAX; ++i) {
			if (event_test_bit(i, key_bitmask)) {
				unsigned j;

				/* search for the override */
				for (j = override_begin; j < override_end; ++j) {
					if (override_map[j].index != index)
						continue;
					break;
				}

				/* if found, map the button */
				if (j < override_end) {
					const char* name = override_map[j].name;
					int bind = override_map[j].bind;

					item->button_map[item->button_mac].code = i;
					item->button_map[item->button_mac].state = 0;
					item->button_map[item->button_mac].sort_order = j;
					item->button_map[item->button_mac].revbind = bind;

					sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), name);

					++item->button_mac;
					if (item->button_mac >= EVENT_JOYSTICK_BUTTON_MAX)
						break;
				}

				/* increment the index even if the button is unused */
				++index;
			}
		}

		/* sort the button in the defined order */
		qsort(item->button_map, item->button_mac, sizeof(item->button_map[0]), button_compare);

		/* after sorting add the binding */
		for (i = 0; i < item->button_mac; ++i) {
			item->bind_map[item->button_map[i].revbind] = i;
			item->bind_map[i] = i;
		}
	} else {
		/* iterate only over recognized buttons */
		item->button_mac = 0;
		for (i = 0; i < sizeof(button_map) / sizeof(button_map[0]); ++i) {
			if (event_test_bit(button_map[i].code, key_bitmask)) {
				unsigned j;
				const char* name = button_map[i].name;
				int bind = button_map[i].bind;
				int has_device = 0;

				item->button_map[item->button_mac].code = button_map[i].code;
				item->button_map[item->button_mac].state = 0;
				item->button_map[item->button_mac].sort_order = i;

				sncpy(item->button_map[item->button_mac].name, sizeof(item->button_map[item->button_mac].name), name);
				if (bind) {
					item->bind_map[bind] = item->button_mac;
					item->button_map[item->button_mac].revbind = bind;
				} else {
					item->button_map[item->button_mac].revbind = -1;
				}

				item->bind_map[item->button_mac] = item->button_mac;
				++item->button_mac;

				if (item->button_mac >= EVENT_JOYSTICK_BUTTON_MAX)
					break;
			}
		}
	}

	memset(abs_bitmask, 0, sizeof(abs_bitmask));
	if (event_test_bit(EV_ABS, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_ABS, sizeof(abs_bitmask)), abs_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_ABS,%d))\n", (int)ABS_MAX));
			return -1;
		}
	}

	item->stick_mac = 0;
	for (i = 0; i < sizeof(stick_map) / sizeof(stick_map[0]); ++i) {
		if (item->stick_mac < EVENT_JOYSTICK_STICK_MAX) {
			unsigned j;
			struct joystick_stick_context* stick = item->stick_map + item->stick_mac;

			stick->axe_mac = 0;
			sncpy(stick->name, sizeof(stick->name), stick_map[i].name);

			for (j = 0; stick_map[i].axe_map[j].code != ABS_UNASSIGNED; ++j) {
				int code = stick_map[i].axe_map[j].code;
				if (event_test_bit(code, abs_bitmask)) {
					if (stick->axe_mac < EVENT_JOYSTICK_AXE_MAX) {
						struct joystick_axe_context* axe = stick->axe_map + stick->axe_mac;
						int features[5];

						axe->code = code;

						sncpy(axe->name, sizeof(axe->name), stick_map[i].axe_map[j].name);

						memset(features, 0, sizeof(features));
						if (ioctl(f, EVIOCGABS(code), features) < 0) {
							axe->min = 0;
							axe->max = 0;
							axe->fuzz = 0;
							axe->flat = 0;
							axe->digit_low = -32;
							axe->digit_high = 32;
							axe->value = 0;
							axe->value_adj = 0;
						} else {
							int middle = (features[1] + features[2]) / 2;
							int size = features[2] - features[1];
							int flat = features[4];
							axe->min = features[1];
							axe->max = features[2];
							axe->fuzz = features[3];
							axe->flat = features[4];
							axe->digit_low = middle - flat - (size - flat) / 8;
							axe->digit_high = middle + flat + (size - flat) / 8;
							axe->value = middle;
							axe->value_adj = 0;
						}

						++stick->axe_mac;
					}
				}
			}

			/* save the stick only if it hash some axes */
			if (stick->axe_mac)
				++item->stick_mac;
		}
	}

	memset(rel_bitmask, 0, sizeof(rel_bitmask));
	if (event_test_bit(EV_REL, item->evtype_bitmask)) {
		if (ioctl(f, EVIOCGBIT(EV_REL, sizeof(rel_bitmask)), rel_bitmask) < 0) {
			log_std(("event: error in ioctl(EVIOCGBIT(EV_REL,%d))\n", (int)REL_MAX));
			return -1;
		}
	}

	item->rel_mac = 0;
	for (i = 0; i < sizeof(rel_map) / sizeof(rel_map[0]); ++i) {
		if (event_test_bit(rel_map[i].code, rel_bitmask)) {
			if (item->rel_mac < EVENT_JOYSTICK_REL_MAX) {
				item->rel_map[item->rel_mac].code = rel_map[i].code;
				item->rel_map[item->rel_mac].value = 0;
				sncpy(item->rel_map[item->rel_mac].name, sizeof(item->rel_map[item->rel_mac].name), rel_map[i].name);
				++item->rel_mac;
			}
		}
	}

	return 0;
}

adv_error joystickb_event_init(int joystickb_id)
{
	unsigned i;
	adv_bool eacces = 0;
	struct event_location map[EVENT_JOYSTICK_DEVICE_MAX];
	unsigned mac;

	log_std(("josytickb:event: joystickb_event_init(id:%d)\n", joystickb_id));

	log_std(("joystickb:event: opening joystick from 0 to %d\n", EVENT_JOYSTICK_DEVICE_MAX));

	event_state.counter = 0;

	mac = event_locate(map, EVENT_JOYSTICK_DEVICE_MAX, "event", &eacces);

	event_state.mac = 0;
	for (i = 0; i < mac; ++i) {
		int f;
		struct joystick_item_context* item = &event_state.map[event_state.mac];

		if (event_state.mac >= EVENT_JOYSTICK_MAX)
			continue;

		f = event_open(map[i].file, item->evtype_bitmask, sizeof(item->evtype_bitmask));
		if (f == -1) {
			if (errno == EACCES) {
				eacces = 1;
			}
			continue;
		}

		if (!event_is_joystick(f, item->evtype_bitmask)) {
			log_std(("joystickb:event: not a joystick on device %s\n", map[i].file));
			event_close(f);
			continue;
		}

		sncpy(item->desc, sizeof(item->desc), map[i].desc);
		item->vendor = map[i].vendor;
		item->product = map[i].product;
		item->version = map[i].version;
		item->bus = map[i].bus;

		if (joystickb_setup(item, f) != 0) {
			event_close(f);
			continue;
		}

		++event_state.mac;
	}

	if (!event_state.mac) {
		if (eacces)
			error_set("No joystick found. Check the /dev/input/event* permissions.\n");
		else
			error_set("No joystick found.\n");
		return -1;
	}

	return 0;
}

void joystickb_event_done(void)
{
	unsigned i;

	log_std(("josytickb:event: joystickb_event_done()\n"));

	for (i = 0; i < event_state.mac; ++i)
		event_close(event_state.map[i].f);
	event_state.mac = 0;
}

unsigned joystickb_event_count_get(void)
{
	log_debug(("joystickb:event: joystickb_event_count_get()\n"));

	return event_state.mac;
}

int joystickb_event_device_name_get(unsigned joystick, char* name)
{
	log_debug(("joystickb:event: joystickb_device_event_name_get(%u)\n", joystick));

	if (event_state.map[joystick].vendor == 0)
		return -1;

	snprintf(name, DEVICE_NAME_MAX, "%04x_%04x", event_state.map[joystick].vendor, event_state.map[joystick].product);

	return 0;
}

int joystickb_event_device_desc_get(unsigned joystick, char* desc)
{
	log_debug(("joystickb:event: joystickb_device_event_desc_get(%u)\n", joystick));

	if (event_state.map[joystick].desc[0] == 0)
		return -1;

	sncpy(desc, DEVICE_NAME_MAX, event_state.map[joystick].desc);

	return 0;
}

unsigned joystickb_event_stick_count_get(unsigned joystick)
{
	log_debug(("joystickb:event: joystickb_event_stick_count_get()\n"));

	return event_state.map[joystick].stick_mac;
}

unsigned joystickb_event_stick_axe_count_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:event: joystickb_event_stick_axe_count_get()\n"));

	return event_state.map[joystick].stick_map[stick].axe_mac;
}

const char* joystickb_event_stick_name_get(unsigned joystick, unsigned stick)
{
	log_debug(("joystickb:event: joystickb_event_stick_name_get()\n"));

	return event_state.map[joystick].stick_map[stick].name;
}

const char* joystickb_event_stick_axe_name_get(unsigned joystick, unsigned stick, unsigned axe)
{
	log_debug(("joystickb:event: joystickb_event_stick_axe_name_get()\n"));

	return event_state.map[joystick].stick_map[stick].axe_map[axe].name;
}

unsigned joystickb_event_stick_axe_digital_get(unsigned joystick, unsigned stick, unsigned axe, unsigned d)
{
	int r;
	log_debug(("joystickb:event: joystickb_event_stick_axe_digital_get()\n"));

	r = event_state.map[joystick].stick_map[stick].axe_map[axe].value;
	if (d)
		return r < event_state.map[joystick].stick_map[stick].axe_map[axe].digit_low;
	else
		return r > event_state.map[joystick].stick_map[stick].axe_map[axe].digit_high;
}

int joystickb_event_stick_axe_analog_get(unsigned joystick, unsigned stick, unsigned axe)
{
	int r;
	log_debug(("joystickb:event: joystickb_event_stick_axe_analog_get()\n"));

#ifdef USE_ACTLABS_HACK
	if (stick == 0
		&& axe < 2
		&& event_state.map[joystick].actlabs_hack_enable) {
		return -JOYSTICK_DRIVER_BASE;
	}
#endif

	r = event_state.map[joystick].stick_map[stick].axe_map[axe].value_adj;

	return r;
}

unsigned joystickb_event_button_count_get(unsigned joystick)
{
	log_debug(("joystickb:event: joystickb_event_button_count_get()\n"));

	return event_state.map[joystick].button_mac;
}

const char* joystickb_event_button_name_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:event: joystickb_event_button_name_get()\n"));

	return event_state.map[joystick].button_map[button].name;
}

unsigned joystickb_event_button_get(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:event: joystickb_event_button_get()\n"));

#ifdef USE_ACTLABS_HACK
	if (button == 0
		&& event_state.map[joystick].actlabs_hack_enable
		&& event_state.counter >= event_state.map[joystick].actlabs_hack_counter + ACTLABS_FRAME_DELAY) {
		/* delay at least ACTLABS_FRAME_DELAY poll call before enabling the fake button */
		/* otherwise some games cannot detect the gun movement in time to recognize the shot out-of-screen */
		return 1;
	}

	if (button == 1
		&& event_state.map[joystick].actlabs_hack_enable) {
		return 0;
	}
#endif

	return event_state.map[joystick].button_map[button].state != 0;
}

unsigned joystickb_event_rel_count_get(unsigned joystick)
{
	log_debug(("joystickb:event: joystickb_event_rel_count_get()\n"));

	return event_state.map[joystick].rel_mac;
}

const char* joystickb_event_rel_name_get(unsigned joystick, unsigned rel)
{
	log_debug(("joystickb:event: joystickb_event_button_rel_get()\n"));

	return event_state.map[joystick].rel_map[rel].name;
}

int joystickb_event_rel_get(unsigned joystick, unsigned rel)
{
	int r;

	log_debug(("joystickb:event: joystickb_event_rel_get()\n"));

	r = event_state.map[joystick].rel_map[rel].value;
	event_state.map[joystick].rel_map[rel].value = 0;

	return r;
}

static void joystickb_event_axe_set(struct joystick_axe_context* axe, int value)
{
/*
   The min_value is the minimum value that this particular axis can return, while
   the max_value is the maximum value that it can return. The fuzz element is the
   range of values that can be considered the same (due to mechanical sensor
   tolerances, or some other reason), and is zero for most devices. The flat is the
   range of values about the mid-point in the axes that are indicate a zero
   response (typically, this is the "dead zone" around the null position of a
   joystick).
 */
	int min = axe->min;
	int max = axe->max;
	int fuzz = axe->fuzz;
	int flat = axe->flat;

	axe->value = value;

	if (min >= max) {
		/* adjustment not possible */
		axe->value_adj = 0;
		return;
	}

	if (fuzz) {
		/* detect edge values with some error */

		int fuzz_min = min + fuzz;
		int fuzz_max = max - fuzz;

		if (value < fuzz_min) {
			axe->value_adj = -JOYSTICK_DRIVER_BASE;
			return;
		}

		if (value > fuzz_max) {
			axe->value_adj = JOYSTICK_DRIVER_BASE;
			return;
		}
	}

	if (flat) {
		/* remove the dead zone */

		int middle = (max + min) / 2;
		int flat_min = middle - flat;
		int flat_max = middle + flat;

		if (flat_min <= value && value <= flat_max) {
			/* center position */
			axe->value_adj = 0;
			return;
		}

		if (min + 2 * flat >= max) {
			/* adjustment not possible */
			axe->value_adj = 0;
			return;
		}

		min += flat;
		max -= flat;

		if (value < middle) {
			value += flat;
		} else {
			value -= flat;
		}
	}

	axe->value_adj = joystickb_adjust_analog(value, min, max);
}

int joystickb_event_bind(unsigned joystick, unsigned code)
{
	log_debug(("joystickb:event: joystickb_event_bind_map()\n"));

	return event_state.map[joystick].bind_map[code];
}

int joystickb_event_revbind(unsigned joystick, unsigned button)
{
	log_debug(("joystickb:event: joystickb_event_revbind()\n"));

	return event_state.map[joystick].button_map[button].revbind;
}

int joystickb_event_poll(void)
{
	unsigned i;
	int type, code, value;
	int error = 0;

	log_debug(("joystickb:event: joystickb_event_poll()\n"));

	++event_state.counter;

	for (i = 0; i < event_state.mac; ++i) {
		struct joystick_item_context* item = event_state.map + i;
		int ret;
		while ((ret = event_read(item->f, &type, &code, &value)) == 1) {

			if (type == EV_KEY) {
				unsigned j;

				log_debug(("joystickb:event: read type %d (key), code %d, value %d\n", type, code, value));

				for (j = 0; j < item->button_mac; ++j) {
					if (code == item->button_map[j].code) {
						item->button_map[j].state = value != 0;
						break;
					}
				}
#ifdef USE_ACTLABS_HACK
				/* recogize the special button and enable the hack */
				if (item->vendor == ACTLABS_VENDOR
					&& (item->product == ACTLABS_DEVICE_1 || item->product == ACTLABS_DEVICE_2)
					&& code == ACTLABS_BUTTON) {
					if (value) {
						item->actlabs_hack_enable = 1;
						item->actlabs_hack_counter = event_state.counter;
					} else {
						item->actlabs_hack_enable = 0;
					}
				}
#endif
			} else if (type == EV_REL) {
				unsigned j;

				log_debug(("joystickb:event: read type %d (rel), code %d, value %d\n", type, code, value));

				for (j = 0; j < item->rel_mac; ++j) {
					if (code == item->rel_map[j].code) {
						item->rel_map[j].value += value;
						break;
					}
				}
			} else if (type == EV_ABS) {

				log_debug(("joystickb:event: read type %d (abs), code %d, value %d\n", type, code, value));

				unsigned j;
				for (j = 0; j < item->stick_mac; ++j) {
					unsigned k;
					struct joystick_stick_context* stick = item->stick_map + j;
					for (k = 0; k < stick->axe_mac; ++k) {
						struct joystick_axe_context* axe = stick->axe_map + k;
						if (code == axe->code)
							joystickb_event_axe_set(axe, value);
					}
				}
			} else {
				log_debug(("joystickb:event: read type %d (unknown), code %d, value %d\n", type, code, value));
			}
		}

		/* keep track of any error */
		if (ret < 0)
			error = -1;
	}

	return error;
}

unsigned joystickb_event_flags(void)
{
	return 0;
}

adv_error joystickb_event_load(adv_conf* context)
{
	return 0;
}

void joystickb_event_reg(adv_conf* context)
{
}

/***************************************************************************/
/* Driver */

joystickb_driver joystickb_event_driver = {
	"event",
	DEVICE,
	joystickb_event_load,
	joystickb_event_reg,
	joystickb_event_init,
	joystickb_event_done,
	0,
	0,
	joystickb_event_flags,
	joystickb_event_count_get,
	joystickb_event_stick_count_get,
	joystickb_event_stick_axe_count_get,
	joystickb_event_stick_name_get,
	joystickb_event_stick_axe_name_get,
	joystickb_event_stick_axe_digital_get,
	joystickb_event_stick_axe_analog_get,
	joystickb_event_button_count_get,
	joystickb_event_button_name_get,
	joystickb_event_button_get,
	joystickb_event_rel_count_get,
	joystickb_event_rel_name_get,
	joystickb_event_rel_get,
	0,
	0,
	joystickb_event_poll,
	joystickb_event_device_name_get,
	joystickb_event_device_desc_get,
	joystickb_event_bind,
	joystickb_event_revbind
};

