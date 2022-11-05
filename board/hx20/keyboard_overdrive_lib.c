#include "ko_platform.h"

#define IS_TAP_HOLD_ACTION(keycode) ((KEY_GET_OP(keycode) == OP_MOD_TAP || KEY_GET_OP(keycode) == OP_LAYER_TAP) && KEY_GET_KC(keycode) != KC_NO)

static layer_state_t base_layers   = 0b00000001;
static layer_state_t active_layers = 0b00000000;

// This is used to cache which layer a pressed key came from
#ifdef KO_COMPRESSED_CACHE
// KO_COMPRESSED_CACHE costs 128 bytes of flash but saves 80 bytes of RAM.
// Uncompressed table: 128 bytes (RAM)
//   Compressed table: 48  bytes (RAM)
//       Bitwise code: 128 bytes extra (Flash)
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
#else
uint8_t act_pressed_layers[KEYBOARD_COLS_MAX][KEYBOARD_ROWS] = {{0}};
static void set_pressed_layer(uint8_t row, uint8_t col, uint8_t layer) {
	act_pressed_layers[col][row] = layer;
}

static uint8_t get_pressed_layer(uint8_t row, uint8_t col) {
	return act_pressed_layers[col][row];
}
#endif

static uint16_t get_keycode_at_pos(uint8_t row, uint8_t col, uint8_t* layer) {
	layer_state_t layers = base_layers | active_layers;

	while(layers) {
		int i = ko_topmost_active_layer(layers); // first non-zero bit = highest active layer
		uint16_t kc = keymaps[i][col][row];
		if (kc == KC_TRANSPARENT) {
			layers ^= (1<<i); // mask it off so we get the next deepest layer
			continue; // peer through this layer
		}
		*layer = i;
		return kc;
	}

	*layer = 0;
	return KC_NO;
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

/// REGION: Record Processing
__attribute__((weak)) bool process_record_proto(uint16_t keycode, keyrecord_t* record) { return true; }
__attribute__((weak)) bool process_record_kb(uint16_t keycode, keyrecord_t* record) { return true; }
__attribute__((weak)) bool process_record_user(uint16_t keycode, keyrecord_t* record) { return true; }

bool process_record(uint16_t keycode, struct key_record* record) {
	if (!keycode)
		return false;

	// The user routine gets the highest precedence
	if (!process_record_user(keycode, record)) {
		return false;
	}

	// ... then the keyboard
	if (!process_record_kb(keycode, record)) {
		return false;
	}

	// ... then the protocol
	if (!process_record_proto(keycode, record)) {
		return false;
	}

	// ... then the internal handler.
	switch (KEY_GET_OP(keycode)) {
		case OP_NONE: {
			uint8_t mods = KEY_GET_MOD(keycode);
			if (mods && record->event.pressed) // Send before on press
				ko_send_modifiers(mods, record);
			ko_send_keycode(keycode, record);
			if (mods && !record->event.pressed) // Send after on release
				ko_send_modifiers(mods, record);
			return false;
		}
		case OP_MOD_TAP: {
			if (record->tap.count == 0) { // if held
				ko_send_modifiers(KEY_GET_MOD(keycode), record);
			} else { // if simply pressed
				ko_send_keycode(keycode, record);
			}
			return false;
		} case OP_LAYER_TAP: {
			if (record->tap.count == 0) { // if held
				uint8_t layer = KEY_GET_LAYER(keycode);
				layer_state_set(active_layers ^ ((-(record->event.pressed != 0) ^ active_layers) & (1<<layer)));
			} else {
				ko_send_keycode(keycode, record);
			}
			return false;
		}
		case OP_LAYER_TOGGLE: {
			uint8_t layer = KEY_GET_LAYER(keycode);
			if (record->event.pressed) {
				layer_invert(layer);
			} // toggle actions take effect on press, not release
			return false;
		}
	}
	return false;
}

static void process_tap_hold_action(uint16_t keycode, struct key_record* record) {
	if (record->event.pressed) {
		ko_enqueue_tap_hold_event(keycode, record);
	} else {
		// if it was queued, cancel it and process a tap fire release
		if (ko_cancel_tap_hold_event(keycode, record)) {
			// cancel it
			record->event.pressed = 1;
			record->tap.count = 1;
			process_record(keycode, record);
		}
		// if we can't find it, it already fired. send a release for the modifier or layer
		// tap will be set to 1 if we found a press-fire already scheduled and 0 if we did not
		record->event.pressed = 0;
		process_record(keycode, record);
	}
}

static struct key_record record;

typedef uint8_t ternary_t;
enum _ternary_t {
	T_NOT_INSTALLED = 0,
	T_SEND_EVENT = 1,
	T_DROP_EVENT = 2
};
ternary_t matrix_callback_overload(int8_t row, int8_t col, int8_t pressed, uint16_t* make_code) {
	uint16_t keycode;

	if (!ko_is_enabled()) {
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
	if (IS_TAP_HOLD_ACTION(keycode)) {
		process_tap_hold_action(keycode, &record);
		return T_DROP_EVENT;
	}

	if (!process_record(keycode, &record)) {
		return T_DROP_EVENT;
	}

	return T_DROP_EVENT;
}
/// REGION END

__attribute__((weak)) void ko_suspend_kb(void) { }
__attribute__((weak)) void ko_suspend_user(void) { }
__attribute__((weak)) void ko_resume_kb(void) { }
__attribute__((weak)) void ko_resume_user(void) { }
