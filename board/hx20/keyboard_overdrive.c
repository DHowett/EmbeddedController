#include "common.h"
#include "chipset.h"
#include "hooks.h"
#include "system.h"
#include "stdbool.h"
#include "host_command.h"
#include "keyboard_overdrive.h"

// hx20 specific
#include "i2c_hid_mediakeys.h"
#include "keyboard_8042_sharedlib.h"
#include "keyboard_backlight.h"

enum _ext_keycode {
	FK_RFKL = SAFE_AREA, // RF Kill (Airplane Mode)
	FK_PROJ, // Project (Win+P)
	FK_BRNU, // Brightness Up
	FK_BRND, // Brightness Down
	FK_BKLT, // Keyboard Backlight
	FK_FN,
};

enum _layers {
	_BASE,
	_FN_ANY,
	_FN_PRESSED,
};

#define FK_FLCK TG(_FN_ANY)

const uint16_t keymaps[][KEYBOARD_COLS_MAX][KEYBOARD_ROWS] = {
        [_BASE] = LAYOUT_framework_iso(
                KC_ESC,       KC_MUTE, KC_VOLD, KC_VOLU, KC_MPRV, KC_MPLY, KC_MNXT, FK_BRND, FK_BRNU, FK_PROJ, FK_RFKL, KC_PSCR, KC_MSEL,    KC_DEL,
                KC_GRV,     KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,       KC_BS,
                KC_TAB,       KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,
                KC_CAPS,        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_BSLS,  KC_ENT,
                KC_LSFT, KC_NUBS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                  KC_RSFT,
                                                                                                                                    KC_UP,
                KC_LCTL, FK_FN,   KC_LWIN, KC_LALT,                       KC_SPC,                        KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT
        ),
        [_FN_ANY] = LAYOUT_framework_iso(
                _______,      KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,     _______,
                _______,    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,      _______,
                _______,      _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,  _______,
                _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                  _______,
                                                                                                                                    _______,
                _______, _______, _______, _______,                       _______,                       _______, _______, _______, _______, _______
        ),
        [_FN_PRESSED] = LAYOUT_framework_iso(
                FK_FLCK,      _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,    KC_INS,
                _______,    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,      _______,
                _______,      _______, _______, _______, _______, _______, _______, _______, _______, _______, KC_PAUS, _______, _______,
                _______,        _______, _______, _______, _______, _______, _______, _______, KC_SLCK, _______, _______, _______, _______,  _______,
                _______, _______, _______, _______, _______, _______, KC_CTBR, _______, _______, _______, _______, _______,                  _______,
                                                                                                                                    KC_PGUP,
                _______, _______, _______, _______,                       FK_BKLT,                       _______, _______, KC_HOME, KC_PGDN, KC_END 
        ),
};

#define CPUTS(outstr) cputs(CC_KEYBOARD, outstr)
#define CPRINTS(format, args...) cprints(CC_KEYBOARD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_KEYBOARD, format, ## args)

static uint8_t scancode_pause[] = {0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77};
static uint8_t scancode_break[] = {0xE0, 0x7E, 0xE0, 0xF0, 0x7E};
static uint8_t scancode_prtsc_make[]  = {0xE0, 0x12, 0xE0, 0x7C};
static uint8_t scancode_prtsc_break[] = {0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12};

bool process_record_kb(uint16_t keycode, keyrecord_t* record) {
	switch (keycode) {
		case KC_PAUS:
			if (record->event.pressed)
				simulate_scancodes_set2(scancode_pause, sizeof(scancode_pause), 0);
			return false;
		case KC_CTBR:
			if (record->event.pressed)
				simulate_scancodes_set2(scancode_break, sizeof(scancode_break), 0);
			return false;
		case KC_PSCR:
			if (record->event.pressed) {
				simulate_scancodes_set2(scancode_prtsc_make, sizeof(scancode_prtsc_make), 1);
			} else {
				simulate_scancodes_set2(scancode_prtsc_break, sizeof(scancode_prtsc_break), 0);
			}
			return false;
	}
	return true;
}

/*
static uint8_t brightness_levels[] = {
	0,
	20,
	50,
	100
};
*/

enum backlight_brightness {
	KEYBOARD_BL_BRIGHTNESS_OFF = 0,
	KEYBOARD_BL_BRIGHTNESS_LOW = 20,
	KEYBOARD_BL_BRIGHTNESS_MED = 50,
	KEYBOARD_BL_BRIGHTNESS_HIGH = 100,
};

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
        switch (keycode) {
                case FK_FN: { // FN
			// The FN key toggles the top row (F1-F12)
			// which we keep in its own layer.
			// We use XOR here to turn it off while held if function lock
			// has turned it on
			layer_invert(_FN_ANY);
			CPUTS("~LAY 1 ");
			if (record->event.pressed) {
				layer_on(_FN_PRESSED);
				CPUTS("+LAY 2\n");
			} else {
				layer_off(_FN_PRESSED);
				CPUTS("-LAY 2\n");
			}
			return false;
		}
		case FK_PROJ:
			if (record->event.pressed) {
				simulate_keyboard(SCANCODE_LEFT_WIN, 1);
				simulate_keyboard(SCANCODE_P, 1);
			} else {
				simulate_keyboard(SCANCODE_P, 0);
				simulate_keyboard(SCANCODE_LEFT_WIN, 0);
			}
			return false;
		case FK_RFKL: // RF Kill - HID report
			update_hid_key(HID_KEY_AIRPLANE_MODE, record->event.pressed);
			return false;
		case FK_BRND: // Brightness Down - HID report
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_DN, record->event.pressed);
			return false;
		case FK_BRNU: // Brightness Up - HID report
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_UP, record->event.pressed);
			return false;
		case FK_BKLT: { // Backlight - Handled internally
			if (record->event.pressed) {
				uint8_t bl_brightness = kblight_get();
				/*
				int i = 0;
				for(; i < sizeof(brightness_levels); ++i) {
					if (brightness_levels[i] >= bl_brightness)
						break;
				}
				TODO genericize bl level ring
				*/
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
			return false;
		}
        }
	return true;
}

static void keyboard_overdrive_shutdown(void) {
	// Preserve the state of the FN key layer
	system_set_bbram(SYSTEM_BBRAM_IDX_KEYBOARD_OVERDRIVE_STATE, layer_state_is(_FN_ANY));
	layer_off(_FN_ANY);
}
DECLARE_HOOK(HOOK_CHIPSET_SHUTDOWN, keyboard_overdrive_shutdown, HOOK_PRIO_DEFAULT);

void keyboard_overdrive_startup(void) {
	uint8_t current_kb = 0;

	if (system_get_bbram(SYSTEM_BBRAM_IDX_KEYBOARD_OVERDRIVE_STATE, &current_kb) == EC_SUCCESS) {
		if (current_kb) {
			layer_on(_FN_ANY);
		}
	}
}
DECLARE_HOOK(HOOK_CHIPSET_STARTUP, keyboard_overdrive_startup, HOOK_PRIO_DEFAULT);
