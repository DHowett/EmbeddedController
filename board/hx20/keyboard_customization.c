/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "chipset.h"
#include "keyboard_customization.h"
#include "keyboard_8042_sharedlib.h"
#include "keyboard_config.h"
#include "keyboard_protocol.h"
#include "keyboard_raw.h"
#include "keyboard_scan.h"
#include "keyboard_backlight.h"
#include "pwm.h"
#include "hooks.h"
#include "system.h"

#include "i2c_hid_mediakeys.h"
/* Console output macros */
#define CPUTS(outstr) cputs(CC_KEYBOARD, outstr)
#define CPRINTS(format, args...) cprints(CC_KEYBOARD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_KEYBOARD, format, ## args)

#define SCANCODE_CTRL_ESC 0x0101

static uint8_t console_keyboard_mode;
int try_console_enqueue(uint16_t* make_code, int8_t pressed);

uint16_t scancode_set2[KEYBOARD_COLS_MAX][KEYBOARD_ROWS] = {
		{0x0021, 0x007B, 0x0079, 0x0072, 0x007A, 0x0071, 0x0069, 0xe04A},
		{0xe071, 0xe070, 0x007D, 0xe01f, 0x006c, 0xe06c, 0xe07d, 0x0077},
		{0x0015, 0x0070, 0x00ff, 0x000D, 0x000E, 0x0016, 0x0067, 0x001c},
		{0xe011, 0x0011, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0xe05a, 0x0029, 0x0024, 0x000c, 0x0101, 0x0026, 0x0004, 0xe07a},
		{0x0022, 0x001a, 0x0006, 0x0005, 0x001b, 0x001e, 0x001d, 0x0076},
		{0x002A, 0x0032, 0x0034, 0x002c, 0x002e, 0x0025, 0x002d, 0x002b},
		{0x003a, 0x0031, 0x0033, 0x0035, 0x0036, 0x003d, 0x003c, 0x003b},
		{0x0049, 0xe072, 0x005d, 0x0044, 0x0009, 0x0046, 0x0078, 0x004b},
		{0x0059, 0x0012, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x0041, 0x007c, 0x0083, 0x000b, 0x0003, 0x003e, 0x0043, 0x0042},
		{0x0013, 0x0064, 0x0075, 0x0001, 0x0051, 0x0061, 0xe06b, 0xe02f},
		{0xe014, 0x0014, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
		{0x004a, 0xe075, 0x004e, 0x0007, 0x0045, 0x004d, 0x0054, 0x004c},
		{0x0052, 0x005a, 0xe03c, 0xe069, 0x0055, 0x0066, 0x005b, 0x0023},
		{0x006a, 0x000a, 0xe074, 0xe054, 0x0000, 0x006b, 0x0073, 0x0074},
};


uint16_t get_scancode_set2(uint8_t row, uint8_t col)
{
	if (col < KEYBOARD_COLS_MAX && row < KEYBOARD_ROWS)
		return scancode_set2[col][row];
	return 0;
}

void set_scancode_set2(uint8_t row, uint8_t col, uint16_t val)
{
	if (col < KEYBOARD_COLS_MAX && row < KEYBOARD_ROWS)
		scancode_set2[col][row] = val;
}

// void board_keyboard_drive_col(int col)
// {
// 	/* Drive all lines to high */
// 	if (col == KEYBOARD_COLUMN_NONE)
// 		gpio_set_level(GPIO_KBD_KSO4, 0);

// 	/* Set KBSOUT to zero to detect key-press */
// 	else if (col == KEYBOARD_COLUMN_ALL)
// 		gpio_set_level(GPIO_KBD_KSO4, 1);

// 	/* Drive one line for detection */
// 	else {
// 		if (col == 4)
// 			gpio_set_level(GPIO_KBD_KSO4, 1);
// 		else
// 			gpio_set_level(GPIO_KBD_KSO4, 0);
// 	}
// }


#ifdef CONFIG_KEYBOARD_DEBUG
static char keycap_label[KEYBOARD_COLS_MAX][KEYBOARD_ROWS] = {
	{KLLI_UNKNO, KLLI_UNKNO, KLLI_L_CTR, KLLI_SEARC,
			KLLI_R_CTR, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO},
	{KLLI_F11,   KLLI_ESC,   KLLI_TAB,   '~',
			'a',        'z',        '1',        'q'},
	{KLLI_F1,    KLLI_F4,    KLLI_F3,    KLLI_F2,
			'd',        'c',        '3',        'e'},
	{'b',        'g',        't',        '5',
			'f',        'v',        '4',        'r'},
	{KLLI_F10,   KLLI_F7,    KLLI_F6,    KLLI_F5,
			's',        'x',        '2',        'w'},
	{KLLI_UNKNO, KLLI_F12,   ']',        KLLI_F13,
			'k',        ',',        '8',        'i'},
	{'n',        'h',        'y',        '6',
			'j',        'm',        '7',        'u'},
	{KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO,
			KLLI_UNKNO, KLLI_L_SHT, KLLI_UNKNO, KLLI_R_SHT},
	{'=',        '\'',       '[',        '-',
			';',        '/',        '0',        'p'},
	{KLLI_F14,   KLLI_F9,    KLLI_F8,    KLLI_UNKNO,
			'|',        '.',        '9',        'o'},
	{KLLI_R_ALT, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO,
			KLLI_UNKNO, KLLI_UNKNO, KLLI_L_ALT, KLLI_UNKNO},
	{KLLI_F15,   KLLI_B_SPC, KLLI_UNKNO, '\\',
			KLLI_ENTER, KLLI_SPACE, KLLI_DOWN,  KLLI_UP},
	{KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO,
			KLLI_UNKNO, KLLI_UNKNO, KLLI_RIGHT, KLLI_LEFT},
#ifdef CONFIG_KEYBOARD_KEYPAD
	/* TODO: Populate these */
	{KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO,
			KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO},
	{KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO,
			KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO, KLLI_UNKNO},
#endif
};

char get_keycap_label(uint8_t row, uint8_t col)
{
	if (col < KEYBOARD_COLS_MAX && row < KEYBOARD_ROWS)
		return keycap_label[col][row];
	return KLLI_UNKNO;
}

void set_keycap_label(uint8_t row, uint8_t col, char val)
{
	if (col < KEYBOARD_COLS_MAX && row < KEYBOARD_ROWS)
		keycap_label[col][row] = val;
}
#endif

#ifdef CONFIG_CAPSLED_SUPPORT

#define SCROLL_LED BIT(0)
#define NUM_LED BIT(1)
#define CAPS_LED BIT(2)
static uint8_t caps_led_status;


int caps_status_check(void)
{
	return caps_led_status;
}

void hx20_8042_led_control(int data)
{
	if (data & CAPS_LED) {
		caps_led_status = 1;
		gpio_set_level(GPIO_CAP_LED_L, 1);
	} else {
		caps_led_status = 0;
		gpio_set_level(GPIO_CAP_LED_L, 0);
	}
}

void caps_suspend(void)
{
	gpio_set_level(GPIO_CAP_LED_L, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, caps_suspend, HOOK_PRIO_DEFAULT);

void caps_resume(void)
{
	if (caps_status_check())
		gpio_set_level(GPIO_CAP_LED_L, 1);
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, caps_resume, HOOK_PRIO_DEFAULT);

#endif

#ifdef CONFIG_KEYBOARD_BACKLIGHT
enum backlight_brightness {
	KEYBOARD_BL_BRIGHTNESS_OFF = 0,
	KEYBOARD_BL_BRIGHTNESS_LOW = 20,
	KEYBOARD_BL_BRIGHTNESS_MED = 50,
	KEYBOARD_BL_BRIGHTNESS_HIGH = 100,
};

int hx20_kblight_enable(int enable)
{
	/*Sets PCR mask for low power handling*/
	pwm_enable(PWM_CH_KBL, enable);

	return EC_SUCCESS;
}


static int hx20_kblight_set_brightness(int percent)
{
	pwm_set_duty(PWM_CH_KBL, percent);
	return EC_SUCCESS;
}

static int hx20_kblight_get_brightness(void)
{
	return pwm_get_duty(PWM_CH_KBL);
}

static int hx20_kblight_init(void)
{
	pwm_set_duty(PWM_CH_KBL, 0);
	return EC_SUCCESS;
}

const struct kblight_drv kblight_hx20 = {
	.init = hx20_kblight_init,
	.set = hx20_kblight_set_brightness,
	.get = hx20_kblight_get_brightness,
	.enable = hx20_kblight_enable,
};

void board_kblight_init(void)
{
	uint8_t current_kblight = 0;
	if (system_get_bbram(SYSTEM_BBRAM_IDX_KBSTATE, &current_kblight) == EC_SUCCESS)
		kblight_set(current_kblight & 0x7F);
	kblight_register(&kblight_hx20);
	kblight_enable(current_kblight);
}
#endif

#ifdef CONFIG_KEYBOARD_CUSTOMIZATION_COMBINATION_KEY
#define FN_PRESSED BIT(0)
#define FN_LOCKED BIT(1)
static uint8_t Fn_key;
static uint32_t fn_key_table_media;
static uint32_t fn_key_table;

int fn_table_media_set(int8_t pressed, uint32_t fn_bit)
{
	if (pressed) {
		fn_key_table_media |= fn_bit;
		return true;
	} else if (!pressed && (fn_key_table_media & fn_bit)) {
		fn_key_table_media &= ~fn_bit;
		return true;
	}

	return false;
}

int fn_table_set(int8_t pressed, uint32_t fn_bit)
{
	if (pressed && (Fn_key & FN_PRESSED)) {
		fn_key_table |= fn_bit;
		return true;
	} else if (!pressed && (fn_key_table & fn_bit)) {
		fn_key_table &= ~fn_bit;
		return true;
	}

	return false;
}

static void hx20_update_fnkey_led(void) {
	// Update the CAP_LED_L GPIO to On if Fn_key contains FN_LOCKED, Off otherwise
	gpio_set_level(GPIO_CAP_LED_L, (Fn_key & FN_LOCKED) ? 1 : 0);
}

void hx20_fnkey_suspend(void) {
	// Turn out the lights!
	gpio_set_level(GPIO_CAP_LED_L, 0);
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, hx20_fnkey_suspend, HOOK_PRIO_DEFAULT);

void hx20_fnkey_resume(void) {
	hx20_update_fnkey_led();
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, hx20_fnkey_resume, HOOK_PRIO_DEFAULT);

void fnkey_shutdown(void) {
	uint8_t current_kb = 0;

	current_kb |= kblight_get() & 0x7F;

	if (Fn_key & FN_LOCKED) {
		current_kb |= 0x80;
	}
	system_set_bbram(SYSTEM_BBRAM_IDX_KBSTATE, current_kb);

	Fn_key &= ~FN_LOCKED;
	Fn_key &= ~FN_PRESSED;
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, fnkey_shutdown, HOOK_PRIO_DEFAULT);


void fnkey_startup(void) {
	uint8_t current_kb = 0;

	if (system_get_bbram(SYSTEM_BBRAM_IDX_KBSTATE, &current_kb) == EC_SUCCESS) {
		if (current_kb & 0x80) {
			Fn_key |= FN_LOCKED;
		}
	}
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP, fnkey_startup, HOOK_PRIO_DEFAULT);

int hotkey_F1_F12(uint16_t *key_code, uint16_t fn, int8_t pressed)
{
	const uint16_t prss_key = *key_code;

	if (!(Fn_key & FN_LOCKED) &&
		(fn & FN_PRESSED))
		return EC_SUCCESS;
	else if (Fn_key & FN_LOCKED &&
		!(fn & FN_PRESSED) &&
		!fn_key_table_media)
		return EC_SUCCESS;
	else if (!fn_key_table_media && !pressed)
		return EC_SUCCESS;

	switch (prss_key) {
	case SCANCODE_F1:  /* SPEAKER_MUTE */
		if (fn_table_media_set(pressed, KB_FN_F1))
			*key_code = SCANCODE_VOLUME_MUTE;
		break;
	case SCANCODE_F2:  /* VOLUME_DOWN */
		if (fn_table_media_set(pressed, KB_FN_F2))
			*key_code = SCANCODE_VOLUME_DOWN;
		break;
	case SCANCODE_F3:  /* VOLUME_UP */
		if (fn_table_media_set(pressed, KB_FN_F3))
			*key_code = SCANCODE_VOLUME_UP;
		break;
	case SCANCODE_F4:  /* PREVIOUS_TRACK */
		if (fn_table_media_set(pressed, KB_FN_F4))
			*key_code = SCANCODE_PREV_TRACK;
		break;
	case SCANCODE_F5:  /* PLAY_PAUSE */
		if (fn_table_media_set(pressed, KB_FN_F5))
			*key_code = 0xe034;
		break;
	case SCANCODE_F6:  /* NEXT_TRACK */
		if (fn_table_media_set(pressed, KB_FN_F6))
			*key_code = SCANCODE_NEXT_TRACK;
		break;
	case SCANCODE_F7:  /* TODO: DIM_SCREEN */
		if (fn_table_media_set(pressed, KB_FN_F7)) {
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_DN, pressed);
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_F8:  /* TODO: BRIGHTEN_SCREEN */
		if (fn_table_media_set(pressed, KB_FN_F8)) {
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_UP, pressed);
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_F9:  /* EXTERNAL_DISPLAY */
		if (fn_table_media_set(pressed, KB_FN_F9)) {
			if (pressed) {
				simulate_keyboard(SCANCODE_LEFT_WIN, 1);
				simulate_keyboard(SCANCODE_P, 1);
			} else {
				simulate_keyboard(SCANCODE_P, 0);
				simulate_keyboard(SCANCODE_LEFT_WIN, 0);
			}
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_F10:  /* FLIGHT_MODE */
		if (fn_table_media_set(pressed, KB_FN_F10)) {
			update_hid_key(HID_KEY_AIRPLANE_MODE, pressed);
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_F11:
			/*
			 * TODO this might need an
			 * extra key combo of:
			 * 0xE012 0xE07C to simulate
			 * PRINT_SCREEN
			 */
		if (fn_table_media_set(pressed, KB_FN_F11))
			*key_code = 0xE07C;
		break;
	case SCANCODE_F12:  /* TODO: FRAMEWORK */
		/* Media Select scan code */
		if (fn_table_media_set(pressed, KB_FN_F12)) {
			//*key_code = 0xE050;
			if (pressed) {
				console_keyboard_mode = 1;
				ccputs("EC Keyboard Console\n");
				ccputs("> ");
			}
			return EC_ERROR_UNIMPLEMENTED; // eat the event, always
		}
		break;
	default:
		return EC_SUCCESS;
	}
	return EC_SUCCESS;
}


int hotkey_special_key(uint16_t *key_code, int8_t pressed)
{
	const uint16_t prss_key = *key_code;

	switch (prss_key) {
	case SCANCODE_DELETE:  /* TODO: INSERT */
		if (fn_table_set(pressed, KB_FN_DELETE))
			*key_code = 0xe070;
		break;
	case SCANCODE_K:  /* TODO: SCROLL_LOCK */
		if (fn_table_set(pressed, KB_FN_K))
			*key_code = SCANCODE_SCROLL_LOCK;
		break;
	case SCANCODE_S:  /* TODO: SYSRQ */
		/*if (!fn_table_set(pressed, KB_FN_S))*/

		break;
	case SCANCODE_LEFT:  /* HOME */
		if (fn_table_set(pressed, KB_FN_LEFT))
			*key_code = 0xe06c;
		break;
	case SCANCODE_RIGHT:  /* END */
		if (fn_table_set(pressed, KB_FN_RIGHT))
			*key_code = 0xe069;
		break;
	case SCANCODE_UP:  /* PAGE_UP */
		if (fn_table_set(pressed, KB_FN_UP))
			*key_code = 0xe07d;
		break;
	case SCANCODE_DOWN:  /* PAGE_DOWN */
		if (fn_table_set(pressed, KB_FN_DOWN))
			*key_code = 0xe07a;
		break;
	case SCANCODE_CTRL_ESC:
		*key_code = 0x0058;
		break;
	default:
		return EC_SUCCESS;
	}

	return EC_SUCCESS;
}

int functional_hotkey(uint16_t *key_code, int8_t pressed)
{
	const uint16_t prss_key = *key_code;
	uint8_t bl_brightness = 0;

	switch (prss_key) {
	case SCANCODE_ESC: /* TODO: FUNCTION_LOCK */
		if (fn_table_set(pressed, KB_FN_ESC)) {
			if (pressed) {
				if (Fn_key & FN_LOCKED)
					Fn_key &= ~FN_LOCKED;
				else
					Fn_key |= FN_LOCKED;
			}
			return EC_ERROR_UNIMPLEMENTED;
			hx20_update_fnkey_led();
		}
		break;
	case SCANCODE_B:
		/* BREAK_KEY */
		if (fn_table_set(pressed, KB_FN_B)) {
			if (pressed) {
				simulate_keyboard(0xe07e, 1);
				simulate_keyboard(0xe0, 1);
				simulate_keyboard(0x7e, 0);
			}
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_P:
		/* PAUSE_KEY */
		if (fn_table_set(pressed, KB_FN_P)) {
			if (pressed) {
				simulate_keyboard(0xe114, 1);
				simulate_keyboard(0x77, 1);
				simulate_keyboard(0xe1, 1);
				simulate_keyboard(0x14, 0);
				simulate_keyboard(0x77, 0);
			}
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	case SCANCODE_SPACE:	/* TODO: TOGGLE_KEYBOARD_BACKLIGHT */
		if (fn_table_set(pressed, KB_FN_SPACE)) {
			if (pressed) {
				bl_brightness = kblight_get();
				switch (bl_brightness) {
				case KEYBOARD_BL_BRIGHTNESS_LOW:
					bl_brightness = KEYBOARD_BL_BRIGHTNESS_MED;
					break;
				case KEYBOARD_BL_BRIGHTNESS_MED:
					bl_brightness = KEYBOARD_BL_BRIGHTNESS_HIGH;
					break;
				case KEYBOARD_BL_BRIGHTNESS_HIGH:
					hx20_kblight_enable(0);
					bl_brightness = KEYBOARD_BL_BRIGHTNESS_OFF;
					break;
				default:
				case KEYBOARD_BL_BRIGHTNESS_OFF:
					hx20_kblight_enable(1);
					bl_brightness = KEYBOARD_BL_BRIGHTNESS_LOW;
					break;
				}
				kblight_set(bl_brightness);
			}
			/* we dont want to pass the space key event to the OS */
			return EC_ERROR_UNIMPLEMENTED;
		}
		break;
	}
	return EC_SUCCESS;
}

static uint8_t ctrlesc_state = 0; // 0 = nothing, 1 = down, 2 = ctrl fired
static void ctrlesc_fire_ctrl(void) {
	// After 200ms, if the key is still down, fire a make for ctrl
	if (ctrlesc_state == 1) {
		simulate_keyboard(SCANCODE_LEFT_CTRL, 1); // MAKE CTRL
		ctrlesc_state = 2;
	}

	// if we got here, it accidentally double-fired
}
DECLARE_DEFERRED(ctrlesc_fire_ctrl);

int try_ctrl_esc(uint16_t *key_code, int8_t pressed) {
	if (*key_code == SCANCODE_CTRL_ESC) {
		if (pressed) {
			ctrlesc_state = 1;
			hook_call_deferred(&ctrlesc_fire_ctrl_data, 200000); // 200msec
		} else {
			if (ctrlesc_state == 1) {
				// Cancel the ctrl key firing
				hook_call_deferred(&ctrlesc_fire_ctrl_data, -1);

				// Send a make/break for ESC
				simulate_keyboard(SCANCODE_ESC, 1); // MAKE ESC
				simulate_keyboard(SCANCODE_ESC, 0); // BREAK ESC
			} else if (ctrlesc_state == 2) {
				// We already sent a make for CTRL, let's send a break
				simulate_keyboard(SCANCODE_LEFT_CTRL, 0); // BREAK CTRL
			}
			ctrlesc_state = 0;
		}
		return EC_ERROR_UNIMPLEMENTED; // placeholder to make the scan fail out
	}
	return EC_SUCCESS;
}

enum ec_error_list keyboard_scancode_callback(uint16_t *make_code,
					      int8_t pressed)
{
	const uint16_t pressed_key = *make_code;
	int r = 0;

	if (factory_status())
		return EC_SUCCESS;

	if (pressed_key == SCANCODE_FN && pressed) {
		Fn_key |= FN_PRESSED;
		return EC_ERROR_UNIMPLEMENTED;
	} else if (pressed_key == SCANCODE_FN && !pressed) {
		Fn_key &= ~FN_PRESSED;
		return EC_ERROR_UNIMPLEMENTED;
	}

	/*
	 * If the system still in preOS
	 * then we pass through all events without modifying them
	 */
	if (!pos_get_state())
		return EC_SUCCESS;

	// TODO: make FN+CAPS = Caps (we need to pre-check here whether we're
	// tracking the Fn key state)
	r = try_ctrl_esc(make_code, pressed);
	if (r != EC_SUCCESS)
		return r;

	r = try_console_enqueue(make_code, pressed);
	if (r != EC_SUCCESS)
		return r;

	r = hotkey_F1_F12(make_code, Fn_key, pressed);
	if (r != EC_SUCCESS)
		return r;
	/*
	 * If the function key is not held then
	 * we pass through all events without modifying them
	 * but if last time have press FN still need keep that
	 */
	if (!(Fn_key & FN_PRESSED) && !fn_key_table)
		return EC_SUCCESS;

	r = hotkey_special_key(make_code, pressed);
	if (r != EC_SUCCESS)
		return r;

	r = functional_hotkey(make_code, pressed);
	if (r != EC_SUCCESS)
		return r;

	return EC_SUCCESS;
}
#endif

#ifdef CONFIG_FACTORY_SUPPORT
/* By default the power button is active low */
#ifndef CONFIG_FP_POWER_BUTTON_FLAGS
#define CONFIG_FP_POWER_BUTTON_FLAGS 0
#endif
static uint8_t factory_enable;
static int debounced_fp_pressed;

static void fp_power_button_deferred(void)
{
	keyboard_update_button(KEYBOARD_BUTTON_POWER_FAKE,
			debounced_fp_pressed);
}
DECLARE_DEFERRED(fp_power_button_deferred);

void factory_power_button(int level)
{
	/* Re-enable keyboard scanning if fp power button is no longer pressed */
	if (!level)
		keyboard_scan_enable(1, KB_SCAN_DISABLE_POWER_BUTTON);

	if (level == debounced_fp_pressed) {
		return;
	}
	debounced_fp_pressed = level;

	hook_call_deferred(&fp_power_button_deferred_data, 50);
}

void factory_setting(uint8_t enable)
{
	if (enable) {
		factory_enable = 1;
		debounced_fp_pressed = 1;
		set_scancode_set2(2, 2, SCANCODE_FAKE_FN);
	} else {
		factory_enable = 0;
		debounced_fp_pressed = 0;
		set_scancode_set2(2, 2, SCANCODE_FN);
	}
}

int factory_status(void)
{
	return factory_enable;
}

#endif

#define CON_MOD_SHIFT 0x1
#define CON_MOD_CTRL  0x2
#define CON_MOD_ALT   0x4
#define GET_MOD(v)    ((0xFF00 & (v)) >> 8)
#define SHIFTED(v)    ((CON_MOD_SHIFT << 8) | v)
#define CTRLED(v)     ((CON_MOD_CTRL << 8) | v)
#define ALTED(v)      ((CON_MOD_ALT << 8) | v)
#define CON_BUF_SIZE  16
#define SCANCODE_LEFT_SHIFT 0x12
#define SCANCODE_RIGHT_SHIFT 0x59
static char console_buf[CON_BUF_SIZE];
static uint8_t console_buf_wr, console_buf_rd, console_current_mod;

// no high scancodes here either
char scancode_to_char_map_base[255] = {
	[0x0d] = '\t', [0x0e] = '`', [0x15] = 'q', [0x16] = '1', [0x1a] = 'z', [0x1b] = 's', [0x1c] = 'a', [0x1d] = 'w', [0x1e] = '2', [0x21] = 'c', [0x22] = 'x', [0x23] = 'd', [0x24] = 'e', [0x25] = '4', [0x26] = '3',
	[0x29] = ' ', [0x2a] = 'v', [0x2b] = 'f', [0x2c] = 't', [0x2d] = 'r', [0x2e] = '5', [0x31] = 'n', [0x32] = 'b', [0x33] = 'h', [0x34] = 'g', [0x35] = 'y', [0x36] = '6', [0x3a] = 'm', [0x3b] = 'j', [0x3c] = 'u',
	[0x3d] = '7', [0x3e] = '8', [0x41] = ',', [0x42] = 'k', [0x43] = 'i', [0x44] = 'o', [0x45] = '0', [0x46] = '9', [0x49] = '.', [0x4a] = '/', [0x4b] = 'l', [0x4c] = ';', [0x4d] = 'p', [0x4e] = '-', [0x52] = '\'',
	[0x54] = '[', [0x55] = '=', [0x5a] = '\n', [0x5b] = ']', [0x5d] = '\\', [0x66] = '\x08', [0x76] = '\x1b',
};

char scancode_to_char_map_shift[255] = {
	[0x0e] = '~', [0x15] = 'Q', [0x16] = '1', [0x1a] = 'Z', [0x1b] = 'S', [0x1c] = 'A', [0x1d] = 'W', [0x1e] = '2', [0x21] = 'C', [0x22] = 'X', [0x23] = 'D', [0x24] = 'E', [0x25] = '4', [0x26] = '3', [0x29] = ' ',
	[0x2a] = 'V', [0x2b] = 'F', [0x2c] = 'T', [0x2d] = 'R', [0x2e] = '5', [0x31] = 'N', [0x32] = 'B', [0x33] = 'H', [0x34] = 'G', [0x35] = 'Y', [0x36] = '6', [0x3a] = 'M', [0x3b] = 'J', [0x3c] = 'U', [0x3d] = '7',
	[0x3e] = '8', [0x41] = '<', [0x42] = 'K', [0x43] = 'I', [0x44] = 'O', [0x45] = '0', [0x46] = '9', [0x49] = '>', [0x4a] = '?', [0x4b] = 'L', [0x4c] = ':', [0x4d] = 'P', [0x4e] = '_', [0x52] = '\'', [0x54] = '{',
	[0x55] = '+', [0x5a] = '\n', [0x5b] = '}', [0x5d] = '|', [0x66] = '\x08', [0x76] = '\x1b',
};

char scancode_to_char_map_ctrl[255] = {
	[0x15] = 0x1f & 'Q', [0x16] = 0x1f & '1', [0x1a] = 0x1f & 'Z', [0x1b] = 0x1f & 'S', [0x1c] = 0x1f & 'A', [0x1d] = 0x1f & 'W', [0x1e] = 0x1f & '2', [0x21] = 0x1f & 'C', [0x22] = 0x1f & 'X', [0x23] = 0x1f & 'D', [0x24] = 0x1f & 'E', [0x25] = 0x1f & '4', [0x26] = 0x1f & '3', [0x2a] = 0x1f & 'V', [0x2b] = 0x1f & 'F',
	[0x2c] = 0x1f & 'T', [0x2d] = 0x1f & 'R', [0x2e] = 0x1f & '5', [0x31] = 0x1f & 'N', [0x32] = 0x1f & 'B', [0x33] = 0x1f & 'H', [0x34] = 0x1f & 'G', [0x35] = 0x1f & 'Y', [0x36] = 0x1f & '6', [0x3a] = 0x1f & 'M', [0x3b] = 0x1f & 'J', [0x3c] = 0x1f & 'U', [0x3d] = 0x1f & '7', [0x3e] = 0x1f & '8', [0x42] = 0x1f & 'K',
	[0x43] = 0x1f & 'I', [0x44] = 0x1f & 'O', [0x45] = 0x1f & '0', [0x46] = 0x1f & '9', [0x4b] = 0x1f & 'L', [0x4d] = 0x1f & 'P',
};

// we will deal with high scancodes later
uint16_t char_to_scancode_map[255] = {
	['\t'] = 0x0d, ['`'] = 0x0e, ['q'] = 0x15, ['1'] = 0x16, ['z'] = 0x1a, ['s'] = 0x1b, ['a'] = 0x1c, ['w'] = 0x1d, ['2'] = 0x1e, ['c'] = 0x21, ['x'] = 0x22, ['d'] = 0x23, ['e'] = 0x24, ['4'] = 0x25, ['3'] = 0x26,
	[' '] = 0x29, ['v'] = 0x2a, ['f'] = 0x2b, ['t'] = 0x2c, ['r'] = 0x2d, ['5'] = 0x2e, ['n'] = 0x31, ['b'] = 0x32, ['h'] = 0x33, ['g'] = 0x34, ['y'] = 0x35, ['6'] = 0x36, ['m'] = 0x3a, ['j'] = 0x3b, ['u'] = 0x3c,
	['7'] = 0x3d, ['8'] = 0x3e, [','] = 0x41, ['k'] = 0x42, ['i'] = 0x43, ['o'] = 0x44, ['0'] = 0x45, ['9'] = 0x46, ['.'] = 0x49, ['/'] = 0x4a, ['l'] = 0x4b, [';'] = 0x4c, ['p'] = 0x4d, ['-'] = 0x4e, ['\''] = 0x52,
	['['] = 0x54, ['='] = 0x55, ['\n'] = 0x5a, [']'] = 0x5b, ['\\'] = 0x5d, ['\x08'] = 0x66, ['\x1b'] = 0x76, ['~'] = SHIFTED(0x0e), ['Q'] = SHIFTED(0x15), ['1'] = SHIFTED(0x16), ['Z'] = SHIFTED(0x1a), ['S'] = SHIFTED(0x1b), ['A'] = SHIFTED(0x1c), ['W'] = SHIFTED(0x1d), ['2'] = SHIFTED(0x1e),
	['C'] = SHIFTED(0x21), ['X'] = SHIFTED(0x22), ['D'] = SHIFTED(0x23), ['E'] = SHIFTED(0x24), ['4'] = SHIFTED(0x25), ['3'] = SHIFTED(0x26), [' '] = SHIFTED(0x29), ['V'] = SHIFTED(0x2a), ['F'] = SHIFTED(0x2b), ['T'] = SHIFTED(0x2c), ['R'] = SHIFTED(0x2d), ['5'] = SHIFTED(0x2e), ['N'] = SHIFTED(0x31), ['B'] = SHIFTED(0x32), ['H'] = SHIFTED(0x33),
	['G'] = SHIFTED(0x34), ['Y'] = SHIFTED(0x35), ['6'] = SHIFTED(0x36), ['M'] = SHIFTED(0x3a), ['J'] = SHIFTED(0x3b), ['U'] = SHIFTED(0x3c), ['7'] = SHIFTED(0x3d), ['8'] = SHIFTED(0x3e), ['<'] = SHIFTED(0x41), ['K'] = SHIFTED(0x42), ['I'] = SHIFTED(0x43), ['O'] = SHIFTED(0x44), ['0'] = SHIFTED(0x45), ['9'] = SHIFTED(0x46), ['>'] = SHIFTED(0x49),
	['?'] = SHIFTED(0x4a), ['L'] = SHIFTED(0x4b), [':'] = SHIFTED(0x4c), ['P'] = SHIFTED(0x4d), ['_'] = SHIFTED(0x4e), ['\''] = SHIFTED(0x52), ['{'] = SHIFTED(0x54), ['+'] = SHIFTED(0x55), ['\n'] = SHIFTED(0x5a), ['}'] = SHIFTED(0x5b), ['|'] = SHIFTED(0x5d), [0x1f & 'Q'] = CTRLED(0x15), [0x1f & '1'] = CTRLED(0x16), [0x1f & 'Z'] = CTRLED(0x1a), [0x1f & 'S'] = CTRLED(0x1b),
	[0x1f & 'A'] = CTRLED(0x1c), [0x1f & 'W'] = CTRLED(0x1d), [0x1f & '2'] = CTRLED(0x1e), [0x1f & 'C'] = CTRLED(0x21), [0x1f & 'X'] = CTRLED(0x22), [0x1f & 'D'] = CTRLED(0x23), [0x1f & 'E'] = CTRLED(0x24), [0x1f & '4'] = CTRLED(0x25), [0x1f & '3'] = CTRLED(0x26), [0x1f & 'V'] = CTRLED(0x2a), [0x1f & 'F'] = CTRLED(0x2b), [0x1f & 'T'] = CTRLED(0x2c), [0x1f & 'R'] = CTRLED(0x2d), [0x1f & '5'] = CTRLED(0x2e), [0x1f & 'N'] = CTRLED(0x31),
	[0x1f & 'B'] = CTRLED(0x32), [0x1f & 'H'] = CTRLED(0x33), [0x1f & 'G'] = CTRLED(0x34), [0x1f & 'Y'] = CTRLED(0x35), [0x1f & '6'] = CTRLED(0x36), [0x1f & 'M'] = CTRLED(0x3a), [0x1f & 'J'] = CTRLED(0x3b), [0x1f & 'U'] = CTRLED(0x3c), [0x1f & '7'] = CTRLED(0x3d), [0x1f & '8'] = CTRLED(0x3e), [0x1f & 'K'] = CTRLED(0x42), [0x1f & 'I'] = CTRLED(0x43), [0x1f & 'O'] = CTRLED(0x44), [0x1f & '0'] = CTRLED(0x45), [0x1f & '9'] = CTRLED(0x46),
	[0x1f & 'L'] = CTRLED(0x4b), [0x1f & 'P'] = CTRLED(0x4d),
};

int board_console_getc(void) {
	uint8_t next = (console_buf_rd + 1) % CON_BUF_SIZE;
	int ch = 0;
	if (console_buf_rd == console_buf_wr) {
		return -1;
	}
	ch = console_buf[console_buf_rd];
	console_buf_rd = next;
	return ch;
}

int board_console_putc(int ch) {
	if (console_keyboard_mode) {
		uint16_t v = char_to_scancode_map[ch];

		if (!v) {
			return EC_SUCCESS;
		}
		if (SHIFTED(v)) {
			simulate_keyboard(SCANCODE_LEFT_SHIFT, 1);
		}
		if (CTRLED(v)) {
			simulate_keyboard(SCANCODE_LEFT_CTRL, 1);
		}
		simulate_keyboard(v & 0xff, 1);
		simulate_keyboard(v & 0xff, 0);
		if (CTRLED(v)) {
			simulate_keyboard(SCANCODE_LEFT_CTRL, 0);
		}
		if (SHIFTED(v)) {
			simulate_keyboard(SCANCODE_LEFT_SHIFT, 0);
		}
	}
	return EC_SUCCESS;
}

int try_console_enqueue_inner(uint16_t code, int8_t pressed) {
	char* table = scancode_to_char_map_base;
	char ch = '\0';

	if (console_current_mod & CON_MOD_SHIFT) {
		table = scancode_to_char_map_shift;
	} else if (console_current_mod & CON_MOD_CTRL) {
		table = scancode_to_char_map_ctrl;
	} else if (console_current_mod & CON_MOD_ALT) {
		return EC_SUCCESS; // let this one pass through; alt is an escape hatch to the system
	}

	if (code > 0xFF) {
		return EC_ERROR_UNIMPLEMENTED; // drop
	}

	ch = table[code & 0xFF];

	if (!ch) {
		return EC_ERROR_UNIMPLEMENTED;
	}

	// if the reader catches up to the writer, drop this event
	if (((console_buf_wr + 1) % CON_BUF_SIZE) == console_buf_rd) {
		return EC_ERROR_UNIMPLEMENTED; // drop
	}
	console_buf_wr = (console_buf_wr + 1) % CON_BUF_SIZE;
	console_buf[console_buf_wr] = ch;
	console_has_input();
	return EC_ERROR_UNIMPLEMENTED; // kill the event!
}

int try_console_enqueue(uint16_t* make_code, int8_t pressed) {
	uint16_t code = *make_code;

	if (!console_keyboard_mode) {
		return EC_SUCCESS;
	}

	if (code == SCANCODE_FN)
		return EC_SUCCESS; // let FN through
	
	if (code == SCANCODE_F12 && pressed) {
		console_keyboard_mode = 0; // kill console mode
		return EC_SUCCESS; // let it fall through to emit the press/release
	}

	switch (code) {
		case SCANCODE_LEFT_CTRL:
		case SCANCODE_RIGHT_CTRL:
			if (pressed)
				console_current_mod |= CON_MOD_CTRL;
			else
				console_current_mod &= ~CON_MOD_CTRL;
			break;
		case SCANCODE_LEFT_SHIFT:
		case SCANCODE_RIGHT_SHIFT:
			if (pressed)
				console_current_mod |= CON_MOD_SHIFT;
			else
				console_current_mod &= ~CON_MOD_SHIFT;
			break;
		case SCANCODE_LEFT_ALT:
		case SCANCODE_RIGHT_ALT:
			if (pressed)
				console_current_mod |= CON_MOD_ALT;
			else
				console_current_mod &= ~CON_MOD_ALT;
			break;
		default:
			return try_console_enqueue_inner(code, pressed);

	}

	return EC_ERROR_UNIMPLEMENTED; // EAT THE EVENT, WE SENT IT TO THE CONSOLE
}
