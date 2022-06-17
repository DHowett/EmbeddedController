#include "common.h"
#include "stdbool.h"
#include "system.h"
#include "host_command.h" // prune
#include "queue.h"
#include "task.h"
#include "keyboard_overdrive.h"

#include "keyboard_8042_sharedlib.h"

#define CPUTS(outstr) cputs(CC_KEYBOARD, outstr)
#define CPRINTS(format, args...) cprints(CC_KEYBOARD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_KEYBOARD, format, ## args)

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
	[KC_HOME] = _E0(0x6C), [KC_LALT] = 0x11,      [KC_LBRC] = 0x54,      [KC_LCTL] = 0x14,      [KC_LEFT] = _E0(0x6B), [KC_LSFT] = 0x12,      [KC_LWIN] = _E0(0x1F),
	[KC_MENU] = _E0(0x2F), [KC_MINS] = 0x4E,      [KC_NLCK] = 0x77,      [KC_NUBS] = 0x61,      [KC_QUOT] = 0x52,      [KC_RALT] = _E0(0x11), [KC_RBRC] = 0x5B,
	[KC_RCTL] = _E0(0x14), [KC_RGHT] = _E0(0x74), [KC_RSFT] = 0x59,      [KC_SCLN] = 0x4C,      [KC_SLSH] = 0x4A,      [KC_SPC] = 0x29,       [KC_TAB] = 0x0D,
	[KC_UP] = _E0(0x75),   [KC_F1] = 0x05,        [KC_F2] = 0x06,        [KC_F3] = 0x04,        [KC_F4] = 0x0C,        [KC_F5] = 0x03,        [KC_F6] = 0x0B,
	[KC_F7] = 0x83,        [KC_F8] = 0x0A,        [KC_F9] = 0x01,        [KC_F10] = 0x09,       [KC_F11] = 0x78,       [KC_F12] = 0x07,       [KC_KP_0] = 0x70,
	[KC_KP_1] = 0x69,      [KC_KP_2] = 0x72,      [KC_KP_3] = 0x7A,      [KC_KP_4] = 0x6B,      [KC_KP_5] = 0x73,      [KC_KP_6] = 0x74,      [KC_KP_7] = 0x6C,
	[KC_KP_8] = 0x75,      [KC_KP_9] = 0x7D,      [KC_PAST] = 0x7C,      [KC_PDOT] = 0x71,      [KC_PENT] = _E0(0x5A), [KC_PGDN] = _E0(0x7A), [KC_PGUP] = _E0(0x7D),
	[KC_PINS] = _E0(0x70), [KC_PMNS] = 0x7B,      [KC_PPLS] = 0x79,      [KC_PSLS] = _E0(0x4A), [KC_MSEL] = _E0(0x50), [KC_MNXT] = _E0(0x4D), [KC_MPRV] = _E0(0x15),
	[KC_MPLY] = _E0(0x34), [KC_VOLD] = _E0(0x21), [KC_VOLU] = _E0(0x32), [KC_MUTE] = _E0(0x23), [KC_SLCK] = 0x7E,
	[KC_PSCR] = 0xFC, // E0-7C PrintScr - Might need another special key (E0 12)
	[KC_CTBR] = 0xFE, // E0-7E Ctrl+Break
	[KC_PAUS] = 0xFF, // E1-14-77-E1-F0-14-F0-77 Pause
};
#undef _E0

static struct mutex ko_queue_mutex;
static struct queue ko_queue = QUEUE_NULL(16, struct ko_queued_event);

static layer_state_t base_layers   = 0b00000001;
static layer_state_t active_layers = 0b00000000;

// This is used to cache which layer a pressed key came from
uint8_t act_pressed_layers[(KEYBOARD_COLS_MAX * KEYBOARD_ROWS + 7)/8][LAYER_BITS] = {{0}};

static void set_pressed_layer(uint8_t row, uint8_t col, uint8_t layer) {
	uint8_t keyNum = row * KEYBOARD_COLS_MAX + col;
	int cidx = keyNum / 8;
	int rbit = keyNum % 8;
	for(int i = 0; i < LAYER_BITS; ++i) {
		act_pressed_layers[cidx][i] ^= (-((layer & (1U << i)) != 0) ^ act_pressed_layers[cidx][i]) & (1U << rbit);
	}
}

static uint8_t get_pressed_layer(uint8_t row, uint8_t col) {
	uint8_t keyNum = row * KEYBOARD_COLS_MAX + col;
	int cidx = keyNum / 8;
	int rbit = keyNum % 8;
	uint8_t layer = 0;
	for(int i = 0; i < LAYER_BITS; ++i) {
		layer |= ((act_pressed_layers[cidx][i] & (1U << rbit)) != 0) << i;
	}
	return layer;
}

static uint16_t get_keycode_at_pos(uint8_t row, uint8_t col, uint8_t* layer) {
	uint16_t kc = KC_NO;
	uint8_t layers = base_layers | active_layers;

	*layer = 0;
	for(int i = NUM_LAYERS_MAX - 1; i >= 0; --i) {
		if (layers & (1<<i)) {
			kc = keymaps[i][col][row];
			*layer = i;
			if (kc == KC_TRANSPARENT) {
				continue; // peer through this layer
			}
			goto done;
		}
	}
	kc = KC_NO;
done:
	return kc;
}

__attribute__((weak)) uint8_t layer_state_set_kb(uint8_t state) { return state; }
__attribute__((weak)) uint8_t layer_state_set_user(uint8_t state) { return state; }

void layer_state_set(layer_state_t state) {
	state = layer_state_set_kb(state);
	state = layer_state_set_user(state);
	active_layers = state;
}

void layer_on(uint8_t layer) {
	layer_state_set(active_layers | (1<<layer));
}

void layer_off(uint8_t layer) {
	layer_state_set(active_layers & ~(1<<layer));
}

void layer_invert(uint8_t layer) {
	layer_state_set(active_layers ^ (1<<layer));
}

bool layer_state_cmp(layer_state_t state, uint8_t layer) {
	return (state & (1<<layer)) != 0;
}

bool layer_state_is(uint8_t layer) {
	return layer_state_cmp(active_layers, layer);
}

static void send_kc_sc(uint16_t keycode, struct key_record* record) {
	uint16_t scancode = s_keyCodeToCompressedScanCodeMapping[KEY_GET_KC(keycode)];
	scancode ^= (-((scancode & 0x80) != 0) & 0xE080); // simultaneously sets high 0xE000 and clears 0x80 iff scancode contains 0x80
	if (scancode == 0xe003) scancode = 0x83; // special-case the only value that breaks our optimization
	simulate_keyboard(scancode, record->event.pressed);
}

static uint16_t mod_scancodes[] = {
	[0 /*MOD_LCTL*/] = 0x14,
	[1 /*MOD_LALT*/] = 0x11, 
	[2 /*MOD_LSFT*/] = 0x13,
	[3 /*MOD_LGUI*/] = 0xE01F,
	//[MOD_RCTL] = 0xE014,
	//[MOD_RALT] = 0xE011,
	//[MOD_RGUI] = 0xE027,
};

static void simulate_mods(uint8_t mods, struct key_record* record) {
	for(int i = 0; i < sizeof(mod_scancodes); ++i) {
		if (mods & (1<<i)) {
			simulate_keyboard(mod_scancodes[i], record->event.pressed);
		}
	}
}

__attribute__((weak)) bool process_record_kb(uint16_t keycode, keyrecord_t* record) { return true; }
__attribute__((weak)) bool process_record_user(uint16_t keycode, keyrecord_t* record) { return true; }

bool process_record(uint16_t keycode, struct key_record* record) {

	if (!process_record_kb(keycode, record)) {
		return false;
	}

	if (!process_record_user(keycode, record)) {
		return false;
	}

	switch (KEY_GET_OP(keycode)) {
		case OP_NONE: {
			send_kc_sc(keycode, record);
			return false;
		}
		case OP_MOD_TAP: {
			if (record->tap.count == 0) { // if held
				simulate_mods(KEY_GET_MOD(keycode), record);
			} else { // if simply pressed
				send_kc_sc(keycode, record);
			}
			return false;
		} case OP_LAYER_TAP: {
			if (record->tap.count == 0) { // if held
				uint8_t layer = KEY_GET_LAYER(keycode);
				layer_state_set(active_layers ^ ((-(record->event.pressed != 0) ^ active_layers) & (1<<layer)));
			} else {
				send_kc_sc(keycode, record);
			}
			return false;
		}
		case OP_LAYER_TOGGLE: {
			uint8_t layer = KEY_GET_LAYER(keycode);
			if (record->event.pressed) {
				layer_invert(layer);
				//active_layers ^= 1<<layer;
			} // toggle actions take effect on press, not release
			return false;
		}
	}
	return false;
}

static struct key_record record;
static int8_t global_enable_keyboard_overload = 0;

typedef uint8_t ternary_t;
enum _ternary_t {
	T_NOT_INSTALLED = 0,
	T_SEND_EVENT = 1,
	T_DROP_EVENT = 2
};
ternary_t matrix_callback_overload(int8_t row, int8_t col, int8_t pressed, uint16_t* make_code) {
	uint16_t keycode;

	// return 0 = NOT INSTALLED 1 = make_code normal SC 2 = STOP EVENT
	if (!global_enable_keyboard_overload) {
		return T_NOT_INSTALLED;
	}

	memset(&record, 0, sizeof(record));
	record.event.key.row = row;
	record.event.key.col = col;
	record.event.pressed = pressed != 0;

	if (pressed) {
		uint8_t layer;
		keycode = get_keycode_at_pos(row, col, &layer);
		set_pressed_layer(row, col, layer);
	} else {
		uint8_t layer = get_pressed_layer(row, col);
		keycode = keymaps[layer][col][row];
		set_pressed_layer(row, col, 0);
	}

	/* Early: if keycode is a mod tap, queue it for later */
	if ((KEY_GET_OP(keycode) == OP_MOD_TAP || KEY_GET_OP(keycode) == OP_LAYER_TAP)
			&& KEY_GET_KC(keycode) != KC_NO) {
		if (pressed) {
			// enqueue record for future processing;
			struct ko_queued_event ev = {};
			ev.ts = get_time();
			ev.ts.val += KO_TAP_TERM * MSEC; // fire time
			ev.cancelled = false; // live
			ev.record = record; // copy
			ev.keycode = keycode;
			queue_add_unit(&ko_queue, &ev);
			task_wake(TASK_ID_KEYOVER);
		} else {
			struct queue_iterator it;
			struct ko_queued_event* found = NULL;

			mutex_lock(&ko_queue_mutex);
			queue_begin(&ko_queue, &it);
			while(it.ptr != NULL) {
				struct ko_queued_event* ev = (struct ko_queued_event*)it.ptr;
				if (ev->record.event.key.row == row && ev->record.event.key.col == col) {
					ev->cancelled = true;
					found = ev;
					break;
				}
				queue_next(&ko_queue, &it);
			}
			mutex_unlock(&ko_queue_mutex);

			// if it was queued, cancel it and process a tap fire release
			if (found) {
				// cancel it
				record.event.pressed = 1;
				record.tap.count = 1;
				process_record(keycode, &record);
			}
			// if we can't find it, it already fired. send a release for the modifier or layer
			// tap will be set to 1 if we found a press-fire already scheduled and 0 if we did not
			record.event.pressed = 0;
			process_record(keycode, &record);
		}
		return T_DROP_EVENT;
	}

	if (!process_record(keycode, &record)) {
		return T_DROP_EVENT;
	}

	return T_DROP_EVENT;
}

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
