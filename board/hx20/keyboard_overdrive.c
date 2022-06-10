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

// FK_FN is a layer mask tap for both the ANY and PRESSED layers
//#define FK_FN MO(_FN_PRESSED)
#define FK_FLCK TG(_FN_ANY)

// TODO DH: make sure that FK_FLCK doesn't turn FN_ANY *off* because FK_FN turned it *on*

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

bool process_record_user(uint16_t keycode, uint8_t pressed) {
        switch (keycode) {
                case FK_FN: { // FN
			// The FN key toggles the top row (F1-F12)
			// which we keep in its own layer.
			// We use XOR here to turn it off while held if function lock
			// has turned it on
			layer_invert(_FN_ANY);
			CPUTS("~LAY 1 ");
			if (pressed) {
				layer_on(_FN_PRESSED);
				CPUTS("+LAY 2\n");
			} else {
				layer_off(_FN_PRESSED);
				CPUTS("-LAY 2\n");
			}
			return false;
		}
		case FK_PROJ:
			if (pressed) {
				simulate_keyboard(SCANCODE_LEFT_WIN, 1);
				simulate_keyboard(SCANCODE_P, 1);
			} else {
				simulate_keyboard(SCANCODE_P, 0);
				simulate_keyboard(SCANCODE_LEFT_WIN, 0);
			}
			return false;
		case FK_RFKL: // RF Kill - HID report
			update_hid_key(HID_KEY_AIRPLANE_MODE, pressed);
			return false;
		case FK_BRND: // Brightness Down - HID report
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_DN, pressed);
			return false;
		case FK_BRNU: // Brightness Up - HID report
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_UP, pressed);
			return false;
		case FK_BKLT: // Backlight - Handled internally
			return false;
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
