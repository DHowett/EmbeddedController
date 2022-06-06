#include "common.h"
#include "chipset.h"
#include "hooks.h"
#include "system.h"
#include "stdbool.h"
#include "host_command.h"
#include "keyboard_overdrive.h"

#define CPUTS(outstr) cputs(CC_KEYBOARD, outstr)
#define CPRINTS(format, args...) cprints(CC_KEYBOARD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_KEYBOARD, format, ## args)

static uint8_t base_layers   = 0b00000001;
static uint8_t active_layers = 0b00000000;

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

struct action {
	uint16_t value;
	uint8_t layer;
};

static struct action get_key_action(uint8_t row, uint8_t col) {
	struct action act = {0};
	uint8_t layers = base_layers | active_layers;
	for(int i = NUM_LAYERS_MAX - 1; i >= 0; --i) {
		if (layers & (1<<i)) {
			act.value = keymaps[i][col][row];
			act.layer = i;
			if (act.value == KC_TRANSPARENT) {
				continue; // peer through this layer
			}
			goto done;
		}
	}
	// We didn't hit it somehow?
	act.value = KC_NO;
	act.layer = 0;
done:
	return act;
}

__attribute__((weak)) bool process_record_user(uint16_t keycode, uint8_t pressed) {
	////////////
	if (keycode == FK_FN) {
		// The FN key toggles the top row (F1-F12)
		// which we keep in its own layer.
		// We use XOR here to turn it off while held if function lock
		// has turned it on
		active_layers ^= 1<<_FN_ANY;
		CPUTS("~LAY 1 ");
		if (pressed) {
			active_layers |= 1<<_FN_PRESSED;
			CPUTS("+LAY 2\n");
		} else {
			active_layers &= ~(1<<_FN_PRESSED);
			CPUTS("-LAY 2\n");
		}
		return false;
	}
	////////////
	return true;
}

#if 0
#define _RING_BUF_RD(name) name##_buf_rd
#define _RING_BUF_WR(name) name##_buf_wr
#define RING_BUFFER(type, name, size) \
	static type name##_buf[size]; \
	static uint8_t name##_buf_wr, name##buf_rd; \
	static bool name##_push(const type* v) { \
		if (((_RING_BUF_WR(name) + 1) % size) == _RING_BUF_RD(name)) { \
			return false; \
		} \
		memcpy(&name##_buf[_RING_BUF_WR(name)], v, sizeof(*v)); \
		_RING_BUF_WR(name) = (_RING_BUF_WR(name) + 1) % size; \
		return true; \
	} \
	static bool name##_pop(type* v) { \
		if (_RING_BUF_RD(name) == _RING_BUF_WR(name)) { \
			return false; \
		} \
		memcpy(v, &name##_buf[_RING_BUF_RD(name)], sizeof(*v)); \
		_RING_BUF_RD(name) = (_RING_BUF_RD(name) + 1) % size; \
		return true; \
	}

void process_deferred_key_events(void) {
}
#endif

static int8_t global_enable_keyboard_overload = 0;
// return 0 = NOT INSTALLED 1 = make_code normal SC 2 = STOP EVENT
int8_t process_record_overload(int8_t row, int8_t col, int8_t pressed, uint16_t* make_code) {
	struct action act;
	if (!global_enable_keyboard_overload) {
		return 0;
	}

	CPRINTF("--- KEYSCAN ROW %d COL %d %s LMASK %x ---\n", row, col, pressed?"DOWN":"UP", active_layers|base_layers);
	if (pressed) {
		act = get_key_action(row, col);
		CPRINTF("ACT=%4.04x LAY=%d\n", act.value, act.layer);
		set_pressed_layer(row, col, act.layer);
	} else {
		uint8_t l = get_pressed_layer(row, col);
		CPRINTF("ACT=%4.04x LAY=%d [FORCED]\n", keymaps[l][col][row], l);
		act.value = keymaps[l][col][row];
		act.layer = l;
		set_pressed_layer(row, col, 0);
	}

	if (!process_record_user(act.value, pressed)) {
		return 2;
	}

	switch (KEY_GET_OP(act.value)) {
		case OP_NONE: {
			uint16_t scancode = s_keyCodeToCompressedScanCodeMapping[KEY_GET_KC(act.value)];
			scancode ^= (-((scancode & 0x80) != 0) & 0xE080); // simultaneously sets high 0xE000 and clears 0x80 iff scancode contains 0x80
			if (scancode == 0xe003) scancode = 0x83; // special-case the only value that breaks our optimization
			*make_code = scancode;
			return 1; // SCAN IT OUT LIKE NORMAL
			break;
		}
		case OP_MOD_TAP:
			if (pressed)
				CPUTS("mod tap\n");
			break;
		case OP_LAYER_TAP: {
			uint8_t layer = KEY_GET_LAYER(act.value);
			if (pressed) {
				CPRINTF("+LAY %d\n", layer);
				active_layers |= 1<<layer;
			} else {
				CPRINTF("-LAY %d\n", layer);
				active_layers &= ~(1<<layer);
			}
			break;
		}
		case OP_LAYER_TOGGLE: {
			uint8_t layer = KEY_GET_LAYER(act.value);
			if (pressed) {
				CPRINTF("~LAY %d\n", layer);
				active_layers ^= 1<<layer;
			} // toggle actions take effect on press, not release
			break;
		}
	}
	return 2;
}

#if 0
int main(int argc, char *argv[])
{
	// HIT TEST

	for (int i = 1; i < argc; i += 3) {
		uint8_t pressed = argv[i][0] == 'd';
		uint8_t col = strtol(argv[i+1], NULL, 0);
		uint8_t row = strtol(argv[i+2], NULL, 0);
		keyscan(row, col, pressed);
	}
	for(int i = KC_NO; i < SAFE_AREA; ++i) {
		if (s_keyCodeToCompressedScanCodeMapping[i] == 0) {
			printf("%s missing\n", keyNames[i]);
		} else {
			uint8_t v = s_keyCodeToCompressedScanCodeMapping[i];
			if(v&0x80) {
				printf("%s special: %4.04x\n", keyNames[i], (v&~0x80)|0xE000);
			}
		}
	}

	for(int i = 0; i < 3; ++i) {
		for(int c = 0; c < KEYBOARD_COLS_MAX; ++c) {
			for(int r = 0; r < KEYBOARD_ROWS; ++r) {
				uint16_t v = keymaps[i][c][r];
				if (v == KC_TRANSPARENT || v == KC_NO) continue;
				if (v > sizeof(s_keyCodeToCompressedScanCodeMapping)) {
					printf("%d %d,%d OUT OF RANGE\n", i, c, r);
				} else if (s_keyCodeToCompressedScanCodeMapping[v] == 0) {
					printf("%d %d,%d HAS NO SCANCODE\n", i, c, r);
				}
			}
		}
	}
	return 0;
}
#endif

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
