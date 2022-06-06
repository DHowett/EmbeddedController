#include "ko_platform.h"

#include "queue.h"
#include "task.h"
#include "host_command.h"
#include "keyboard_8042_sharedlib.h"

static int8_t global_enable_keyboard_overload = 0;
bool ko_is_enabled() {
	return global_enable_keyboard_overload;
}

// much easier to have a lookup table (256 bytes) for all keycodes to scancodes.
// Instead of storing the entire raw set2 scancode, which is somewhat expensive,
// we're going to compress it. All of the scancodes we care about range from 0x00
// to 0x8f and 0xe000 to 0xe07f. We have slightly less than one magic bit to work
// with. If bit 0x80 is set, it might be 0x83. Otherwise, we can reserve it as the
// prefix that means |0xE000. There is no 0xE003, so there's no chance of collision.
// We can also reserve 0xFF and 0xFE for the two complicated keys, PAUSE and BREAK,
// which require 6 and 4 byte scancodes respectively. That way lies madness.
//
#define _E0(x) (0x80|(x))
static const uint8_t s_keyCodeToCompressedScanCodeMapping[] = {
	[KC_0] = 0x45,         [KC_1] = 0x16,         [KC_2] = 0x1E,         [KC_3] = 0x26,         [KC_4] = 0x25,         [KC_5] = 0x2E,         [KC_6] = 0x36,
	[KC_7] = 0x3D,         [KC_8] = 0x3E,         [KC_9] = 0x46,         [KC_A] = 0x1C,         [KC_B] = 0x32,         [KC_C] = 0x21,         [KC_D] = 0x23,
	[KC_E] = 0x24,         [KC_F] = 0x2B,         [KC_G] = 0x34,         [KC_H] = 0x33,         [KC_I] = 0x43,         [KC_J] = 0x3B,         [KC_K] = 0x42,
	[KC_L] = 0x4B,         [KC_M] = 0x3A,         [KC_N] = 0x31,         [KC_O] = 0x44,         [KC_P] = 0x4D,         [KC_Q] = 0x15,         [KC_R] = 0x2D,
	[KC_S] = 0x1B,         [KC_T] = 0x2C,         [KC_U] = 0x3C,         [KC_V] = 0x2A,         [KC_W] = 0x1D,         [KC_X] = 0x22,         [KC_Y] = 0x35,
	[KC_Z] = 0x1A,         [KC_BS] = 0x66,        [KC_BSLS] = 0x5D,      [KC_CAPS] = 0x58,      [KC_COMM] = 0x41,      [KC_INS] = _E0(0x70),  [KC_DEL] = _E0(0x71),
	[KC_DOT] = 0x49,       [KC_DOWN] = _E0(0x72), [KC_END] = _E0(0x69),  [KC_ENT] = 0x5A,       [KC_EQL] = 0x55,       [KC_ESC] = 0x76,       [KC_GRV] = 0x0E,
	[KC_HOME] = _E0(0x6C), [KC_LALT] = 0x11,      [KC_LBRC] = 0x54,      [KC_LCTL] = 0x14,      [KC_LEFT] = _E0(0x6B), [KC_LSFT] = 0x12,      [KC_LGUI] = _E0(0x1F),
	[KC_APP] = _E0(0x2F),  [KC_MINS] = 0x4E,      [KC_NLCK] = 0x77,      [KC_NUBS] = 0x61,      [KC_QUOT] = 0x52,      [KC_RALT] = _E0(0x11), [KC_RBRC] = 0x5B,
	[KC_RCTL] = _E0(0x14), [KC_RGHT] = _E0(0x74), [KC_RSFT] = 0x59,      [KC_SCLN] = 0x4C,      [KC_SLSH] = 0x4A,      [KC_SPC] = 0x29,       [KC_TAB] = 0x0D,
	[KC_UP] = _E0(0x75),   [KC_F1] = 0x05,        [KC_F2] = 0x06,        [KC_F3] = 0x04,        [KC_F4] = 0x0C,        [KC_F5] = 0x03,        [KC_F6] = 0x0B,
	[KC_F7] = 0x83,        [KC_F8] = 0x0A,        [KC_F9] = 0x01,        [KC_F10] = 0x09,       [KC_F11] = 0x78,       [KC_F12] = 0x07,       [KC_KP_0] = 0x70,
	[KC_KP_1] = 0x69,      [KC_KP_2] = 0x72,      [KC_KP_3] = 0x7A,      [KC_KP_4] = 0x6B,      [KC_KP_5] = 0x73,      [KC_KP_6] = 0x74,      [KC_KP_7] = 0x6C,
	[KC_KP_8] = 0x75,      [KC_KP_9] = 0x7D,      [KC_PAST] = 0x7C,      [KC_PDOT] = 0x71,      [KC_PENT] = _E0(0x5A), [KC_PGDN] = _E0(0x7A), [KC_PGUP] = _E0(0x7D),
	[KC_PINS] = _E0(0x70), [KC_PMNS] = 0x7B,      [KC_PPLS] = 0x79,      [KC_PSLS] = _E0(0x4A), [KC_MSEL] = _E0(0x50), [KC_MNXT] = _E0(0x4D), [KC_MPRV] = _E0(0x15),
	[KC_MPLY] = _E0(0x34), [KC_VOLD] = _E0(0x21), [KC_VOLU] = _E0(0x32), [KC_MUTE] = _E0(0x23), [KC_SLCK] = 0x7E,      [KC_RGUI] = _E0(0x27),
	[KC_PSCR] = 0xFC, // E0-7C PrintScr - Might need another special key (E0 12)
	[KC_CTBR] = 0xFE, // E0-7E Ctrl+Break
	[KC_PAUS] = 0xFF, // E1-14-77-E1-F0-14-F0-77 Pause
};
#undef _E0

static uint16_t mod_scancodes[] = {
	[0 /*MOD_LCTL*/] = 0x14,
	[1 /*MOD_LALT*/] = 0x11, 
	[2 /*MOD_LSFT*/] = 0x12,
	[3 /*MOD_LGUI*/] = 0xE01F,
	[4 /*MOD_RCTL*/] = 0xE014,
	[5 /*MOD_RALT*/] = 0xE011,
	[6 /*MOD_RSFT*/] = 0x59,
	[7 /*MOD_RGUI*/] = 0xE027,
};

void ko_send_keycode(uint16_t keycode, struct key_record* record) {
	uint16_t scancode = s_keyCodeToCompressedScanCodeMapping[KEY_GET_KC(keycode)];
	scancode ^= (-((scancode & 0x80) != 0) & 0xE080); // simultaneously sets high 0xE000 and clears 0x80 iff scancode contains 0x80
	if (scancode == 0xe003) scancode = 0x83; // special-case the only value that breaks our optimization
	simulate_keyboard(scancode, record->event.pressed);
}

void ko_send_modifiers(uint8_t mods, struct key_record* record) {
	uint8_t offset = (mods & 0b10000) ? 4 : 0; // having any right modifiers makes all modifiers right
	for(int i = 0; i < 4; ++i) {
		if (mods & (1<<i)) {
			simulate_keyboard(mod_scancodes[offset+i], record->event.pressed);
		}
	}
}

struct ko_queued_event {
	timestamp_t ts;
	bool cancelled;
	uint16_t keycode;
	struct key_record record;
};

static struct mutex ko_queue_mutex;
static struct queue ko_queue = QUEUE_NULL(16, struct ko_queued_event);

void ko_enqueue_tap_hold_event(uint16_t keycode, struct key_record* record) {
	// enqueue record for future processing;
	struct ko_queued_event ev = {};
	ev.ts.val = get_time().val + (KO_TAP_TERM * MSEC); // fire time
	ev.cancelled = false; // live
	ev.record = *record; // copy
	ev.keycode = keycode;
	queue_add_unit(&ko_queue, &ev);
	task_wake(TASK_ID_KEYOVER);
}

bool ko_cancel_tap_hold_event(uint16_t keycode, struct key_record* record) {
	struct queue_iterator it;
	struct ko_queued_event* found = NULL;

	mutex_lock(&ko_queue_mutex);
	queue_begin(&ko_queue, &it);
	while(it.ptr != NULL) {
		struct ko_queued_event* ev = (struct ko_queued_event*)it.ptr;
		if (ev->record.event.key.row == record->event.key.row && ev->record.event.key.col == record->event.key.col) {
			ev->cancelled = true;
			found = ev;
			break;
		}
		queue_next(&ko_queue, &it);
	}
	mutex_unlock(&ko_queue_mutex);
	return found != NULL;
}

/// ChromeOS EC PS/2 platform hooks
bool process_record_proto(uint16_t keycode, keyrecord_t* record) {
	switch (keycode) {
		case KC_BRND: // Protocol override: this goes out via HID
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_DN, record->event.pressed);
			return false;
		case KC_BRNU: // Protocol override: this goes out via HID
			update_hid_key(HID_KEY_DISPLAY_BRIGHTNESS_UP, record->event.pressed);
			return false;
		case KC_RFKL: // Protocol override: this goes out via HID
			update_hid_key(HID_KEY_AIRPLANE_MODE, record->event.pressed);
			return false;
	}
	return true;
}

/// REGION: Platform Hooks
//////////////////////////////////
#define EC_CMD_SET_KEYBOARD_OVERDRIVE 0x3E7F

struct ec_params_set_keyboard_overdrive {
	uint8_t on;
} __ec_align1;
static enum ec_status keyboard_overdrive(struct host_cmd_handler_args *args)
{
	const struct ec_params_set_keyboard_overdrive *p = args->params;
	global_enable_keyboard_overload = p->on != 0;
	args->response_size = 0;
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_SET_KEYBOARD_OVERDRIVE, keyboard_overdrive, EC_VER_MASK(0));

static void keyboard_overdrive_suspend(void) {
	ko_suspend_kb();
	ko_suspend_user();
	//dustin consider pretending that we have an eeprom api
}
DECLARE_HOOK(HOOK_CHIPSET_SUSPEND, keyboard_overdrive_suspend, HOOK_PRIO_DEFAULT);

static void keyboard_overdrive_resume(void) {
	ko_resume_kb();
	ko_resume_user();
}
DECLARE_HOOK(HOOK_CHIPSET_RESUME, keyboard_overdrive_resume, HOOK_PRIO_DEFAULT);

void keyboard_overdrive_task(void* u) {
	int wait = -1;
	while(1) {
		struct queue_iterator it;
		timestamp_t t;
		int i = 0;

		task_wait_event(wait); // well, have a nap...

		if (queue_is_empty(&ko_queue)) {
			// if queue is empty, break and go back to sleep
			wait = -1;
			continue;
		}

		mutex_lock(&ko_queue_mutex);
		t = get_time();
		queue_begin(&ko_queue, &it);
		while(it.ptr) { // we will always bail early or run out of queue
			struct ko_queued_event* e = (struct ko_queued_event*)it.ptr;
			// look at e: if it has been cancelled, we can remove it later
			if (e->cancelled) {
				++i; // remove this entry
			} else if (timestamp_expired(e->ts, &t)) {
				struct ko_queued_event copy = *e;
				++i; // remove this entry
				mutex_unlock(&ko_queue_mutex);
				// we don't want to call under lock
				// RISK (TODO): processing a record could modify the queue...
				process_record(copy.keycode, &copy.record);
				mutex_lock(&ko_queue_mutex);
			} else {
				int remaining = e->ts.val - t.val;
				// The first event is not ready yet,
				// so we should wait for it to be ready.
				wait = CLAMP(remaining, 1, KO_TAP_TERM * MSEC);
				break;
			}
			queue_next(&ko_queue, &it);
		}
		queue_advance_head(&ko_queue, i); // delete the cancelled and processed events
		mutex_unlock(&ko_queue_mutex);
	}
}
/// REGION END
