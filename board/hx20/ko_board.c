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
