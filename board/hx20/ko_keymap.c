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
	FK_BKLT = SAFE_AREA, // Keyboard Backlight
	FK_FN,
};

enum _layers {
	_BASE,
	_FN_ANY,
	_FN_PRESSED,
};

#define FK_FLCK TG(_FN_ANY)
#define FK_PROJ ACT_MOD(MOD_LGUI, KC_P) // WIN+P

const uint16_t keymaps[][KEYBOARD_COLS_MAX][KEYBOARD_ROWS] = {
        [_BASE] = LAYOUT_framework_iso(
                KC_ESC,       KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,     KC_DEL,
                KC_GRV,     KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,       KC_BS,
                KC_TAB,       KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,
                KC_CAPS,        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_BSLS,  KC_ENT,
                KC_LSFT, KC_NUBS, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                  KC_RSFT,
                                                                                                                                    KC_UP,
                KC_LCTL, FK_FN,   KC_LWIN, KC_LALT,                       KC_SPC,                        KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT
        ),
        [_FN_ANY] = LAYOUT_framework_iso(
                _______,      KC_MUTE, KC_VOLD, KC_VOLU, KC_MPRV, KC_MPLY, KC_MNXT, KC_BRND, KC_BRNU, FK_PROJ, KC_RFKL, KC_PSCR, KC_MSEL,    _______,
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

enum backlight_brightness {
	KEYBOARD_BL_BRIGHTNESS_OFF = 0,
	KEYBOARD_BL_BRIGHTNESS_LOW = 20,
	KEYBOARD_BL_BRIGHTNESS_MED = 50,
	KEYBOARD_BL_BRIGHTNESS_HIGH = 100,
};

uint8_t layer_state_set_user(uint8_t state) {
	bool light = !layer_state_cmp(state, _FN_ANY);
	gpio_set_level(GPIO_CAP_LED_L, light ? 1 : 0);
	return state;
}

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
        switch (keycode) {
                case FK_FN: { // FN
			// The FN key toggles the top row (F1-F12)
			// which we keep in its own layer.
			// We use XOR here to turn it off while held if function lock
			// has turned it on
			layer_invert(_FN_ANY);
			if (record->event.pressed) {
				layer_on(_FN_PRESSED);
			} else {
				layer_off(_FN_PRESSED);
			}
			return false;
		}
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

void ko_suspend_user(void) {
	// Preserve the state of the FN key layer
	system_set_bbram(SYSTEM_BBRAM_IDX_KEYBOARD_OVERDRIVE_STATE, layer_state_is(_FN_ANY));
	layer_off(_FN_ANY);
}

void ko_resume_user(void) {
	uint8_t current_kb = 0;

	if (system_get_bbram(SYSTEM_BBRAM_IDX_KEYBOARD_OVERDRIVE_STATE, &current_kb) == EC_SUCCESS) {
		if (current_kb) {
			layer_on(_FN_ANY);
		}
	}
}
