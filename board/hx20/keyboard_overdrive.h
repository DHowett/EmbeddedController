#include "stdint.h"

#define KEYBOARD_COLS_MAX 16
#define KEYBOARD_ROWS 8
#define NUM_LAYERS_MAX 8
#define LAYER_BITS 3
#define KO_TAP_TERM 200 /* ms */

enum _opcode {
	OP_NONE         = 0b000, // OP 3, MOD 5, KEY 8
	OP_MOD_TAP      = 0b001, // OP 3, MOD 5, KEY 8
	OP_LAYER_TAP    = 0b010, // OP 3, LAY 5, KEY 8, +press -release
	OP_LAYER_TOGGLE = 0b011, // OP 3, LAY 5, ___ 8, toggles on release
			//0b100
			//0b101
			//0b110
	OP_SPECIAL      = 0b111
};

enum _modifier {
	MOD_LCTL = 0b00001,
	MOD_LALT = 0b00010,
	MOD_LSFT = 0b00100,
	MOD_LGUI = 0b01000,
	MOD_RCTL = 0b10001,
	MOD_RALT = 0b10010,
	MOD_RSFT = 0b10100,
	MOD_RGUI = 0b11000,
	MOD_LWIN = MOD_LGUI,
	MOD_RWIN = MOD_RGUI,
};

#define ACT(op, val) (op<<13|val)
#define ACT_MOD_TAP(mod, key) ACT(OP_MOD_TAP, ((mod)&0x1f)<<8|(key))
#define ACT_LAYER_TAP(lay, key) ACT(OP_LAYER_TAP, ((lay)&0x1f)<<8|(key))
#define ACT_LAYER_TOGGLE(lay) ACT(OP_LAYER_TOGGLE, ((lay)&0x1f)<<8)
#define KEY_GET_OP(kv) (((kv)>>13)&0x7)
#define KEY_GET_LAYER(kv) (((kv)>>8)&0x1f)
#define KEY_GET_KC(kv) ((kv)&0xff)
#define KEY_GET_MOD(kv) (((kv)>>8)&0x1f)
#define ACT_MOD(mod, key) ACT(OP_NONE, ((mod)&0x1f)<<8|(key))

// QMK compatibility
#define MO(layer) ACT_LAYER_TAP(layer, 0)
#define LT(layer, kc) ACT_LAYER_TAP(layer, kc)
#define TG(layer) ACT_LAYER_TOGGLE(layer)
#define MT(mod, kc) ACT_MOD_TAP(mod, kc)

enum _keycode {
	KC_NO = 0,
	KC_TRANSPARENT = 0xFFFF, // OP_SPECIAL | 0x1FFF
	KC_A = 0x1,
	KC_B,
	KC_C,
	KC_D,
	KC_E,
	KC_F,
	KC_G,
	KC_H,
	KC_I,
	KC_J,
	KC_K,
	KC_L,
	KC_M,
	KC_N,
	KC_O,
	KC_P,
	KC_Q,
	KC_R,
	KC_S,
	KC_T,
	KC_U,
	KC_V,
	KC_W,
	KC_X,
	KC_Y,
	KC_Z,

	KC_0,
	KC_1,
	KC_2,
	KC_3,
	KC_4,
	KC_5,
	KC_6,
	KC_7,
	KC_8,
	KC_9,

	KC_BS,
	KC_BSLS,
	KC_CAPS,
	KC_COMM,
	KC_INS,
	KC_DEL,
	KC_DOT,
	KC_DOWN,
	KC_END,
	KC_ENT,
	KC_EQL,
	KC_ESC,
	KC_GRV,
	KC_HOME,
	KC_LALT,
	KC_LBRC,
	KC_LCTL,
	KC_LEFT,
	KC_LSFT,
	KC_LGUI,
	KC_RGUI,
	KC_APP,
	KC_MINS,
	KC_NLCK,
	KC_NUBS,
	KC_QUOT,
	KC_RALT,
	KC_RBRC,
	KC_RCTL,
	KC_RGHT,
	KC_RSFT,
	KC_SCLN,
	KC_SLSH,
	KC_SPC,
	KC_TAB,
	KC_UP,

	KC_F1,
	KC_F2,
	KC_F3,
	KC_F4,
	KC_F5,
	KC_F6,
	KC_F7,
	KC_F8,
	KC_F9,
	KC_F10,
	KC_F11,
	KC_F12,

	KC_KP_0,
	KC_KP_1,
	KC_KP_2,
	KC_KP_3,
	KC_KP_4,
	KC_KP_5,
	KC_KP_6,
	KC_KP_7,
	KC_KP_8,
	KC_KP_9,
	KC_PAST,
	KC_PDOT,
	KC_PENT,
	KC_PGDN,
	KC_PGUP,
	KC_PINS,
	KC_PMNS,
	KC_PPLS,
	KC_PSLS,

	KC_MSEL,
	KC_MNXT,
	KC_MPRV,
	KC_MPLY,

	KC_VOLD,
	KC_VOLU,
	KC_MUTE,

	KC_PAUS,
	KC_CTBR,
	KC_PSCR,
	KC_SLCK,

	KC_BRND,
	KC_BRNU,
	KC_RFKL,

	SAFE_AREA,

	KC_LWIN = KC_LGUI,
	KC_RWIN = KC_RGUI,
};

#define _______ KC_TRANSPARENT

#define KC_MUHENKAN 0x0067
#define KC_KATAKANA_HIRAGANA 0x0013
#define KC_HENKAN            0x0064
#define KC_RO_KANA           0x0051
#define KC_YEN 0x006a

/* KSI0-15 = Columns A-P
 * KSO0-7  = Rows    0-7
 * ISO
	,---------------------------------------------------------------------------.
	| F7    | F3 | F2 | E6 | E3 | K4 | K3 | K2 | P1 | L3 | I4 | I6 | N3 |    B0 |
	|---------------------------------------------------------------------------|
	|   C4 | C5 | F5 | E5 | G5 | G4 | H4 | H5 | K5 | I5 | N4 | N2 | O4 |     O5 |
	|---------------------------------------------------------------------------|
	|    C3 | C0 | F6 | E2 | G6 | G3 | H3 | H6 | K6 | I3 | N5 | N6 | O6 |       |
	|--------------------------------------------------------------------`   O1 |
	| o   E4 | C7 | F4 | O7 | G7 | G2 | H2 | H7 | K7 | I7 | N7 | O0 | I2 |      |
	|---------------------------------------------------------------------------|
	| J1 | L5 | F1 | F0 | A0 | G0 | G1 | H1 | H0 | K0 | I0 | N0 |            J0 |
	|---------------------------------------------------------------------------|
	|    |    |    |    |                              |    |    |    | N1 |    |
	| M1 | C2 | B3 | D1 |              E1              | D0 | M0 | L6 |----| P2 |
	|    |    |    |    |                              |    |    |    | I1 |    |
	`---------------------------------------------------------------------------'

   ANSI
	,---------------------------------------------------------------------------.
	| F7    | F3 | F2 | E6 | E3 | K4 | K3 | K2 | P1 | L3 | I4 | I6 | N3 |    B0 |
	|---------------------------------------------------------------------------|
	|   C4 | C5 | F5 | E5 | G5 | G4 | H4 | H5 | K5 | I5 | N4 | N2 | O4 |     O5 |
	|---------------------------------------------------------------------------|
	|    C3 | C0 | F6 | E2 | G6 | G3 | H3 | H6 | K6 | I3 | N5 | N6 | O6 |    I2 |
	|---------------------------------------------------------------------------|
	| o   E4 | C7 | F4 | O7 | G7 | G2 | H2 | H7 | K7 | I7 | N7 | O0 |        O1 |
	|---------------------------------------------------------------------------|
	|      J1 | F1 | F0 | A0 | G0 | G1 | H1 | H0 | K0 | I0 | N0 |            J0 |
	|---------------------------------------------------------------------------|
	|    |    |    |    |                              |    |    |    | N1 |    |
	| M1 | C2 | B3 | D1 |              E1              | D0 | M0 | L6 |----| P2 |
	|    |    |    |    |                              |    |    |    | I1 |    |
	`---------------------------------------------------------------------------'
*/

#define LAYOUT_framework_iso( \
	KF7,  KF3, KF2, KE6, KE3, KK4, KK3, KK2, KP1, KL3, KI4, KI6, KN3,    KB0, \
	KC4, KC5, KF5, KE5, KG5, KG4, KH4, KH5, KK5, KI5, KN4, KN2, KO4,     KO5, \
	KC3,  KC0, KF6, KE2, KG6, KG3, KH3, KH6, KK6, KI3, KN5, KN6, KO6,         \
	KE4,   KC7, KF4, KO7, KG7, KG2, KH2, KH7, KK7, KI7, KN7, KO0, KI2,   KO1, \
	KJ1, KL5, KF1, KF0, KA0, KG0, KG1, KH1, KH0, KK0, KI0, KN0,          KJ0, \
	                                                                KN1,      \
	KM1, KC2, KB3, KD1,             KE1,             KD0, KM0, KL6, KI1, KP2  \
) { \
	{ KA0,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO }, \
	{ KB0,   KC_NO, KC_NO, KB3,   KC_NO, KC_NO, KC_NO, KC_NO }, \
	{ KC0,   KC_NO, KC2,   KC3,   KC4,   KC5,   KC_NO, KC7   }, \
	{ KD0,   KD1,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO }, \
	{ KC_NO, KE1,   KE2,   KE3,   KE4,   KE5,   KE6,   KC_NO }, \
	{ KF0,   KF1,   KF2,   KF3,   KF4,   KF5,   KF6,   KF7   }, \
	{ KG0,   KG1,   KG2,   KG3,   KG4,   KG5,   KG6,   KG7   }, \
	{ KH0,   KH1,   KH2,   KH3,   KH4,   KH5,   KH6,   KH7   }, \
	{ KI0,   KI1,   KI2,   KI3,   KI4,   KI5,   KI6,   KI7   }, \
	{ KJ0,   KJ1,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO }, \
	{ KK0,   KC_NO, KK2,   KK3,   KK4,   KK5,   KK6,   KK7   }, \
	{ KC_NO, KC_NO, KC_NO, KL3,   KC_NO, KL5,   KL6,   KC_NO }, \
	{ KM0,   KM1,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO }, \
	{ KN0,   KN1,   KN2,   KN3,   KN4,   KN5,   KN6,   KN7   }, \
	{ KO0,   KO1,   KC_NO, KC_NO, KO4,   KO5,   KO6,   KO7   }, \
	{ KC_NO, KP1,   KP2,   KC_NO, KC_NO, KC_NO, KC_NO, KC_NO }, \
}

#define LAYOUT_framework_ansi( \
	KF7,  KF3, KF2, KE6, KE3, KK4, KK3, KK2, KP1, KL3, KI4, KI6, KN3,  KB0, \
	KC4, KC5, KF5, KE5, KG5, KG4, KH4, KH5, KK5, KI5, KN4, KN2, KO4,   KO5, \
	KC3,  KC0, KF6, KE2, KG6, KG3, KH3, KH6, KK6, KI3, KN5, KN6, KO6,  KI2, \
	KE4,   KC7, KF4, KO7, KG7, KG2, KH2, KH7, KK7, KI7, KN7, KO0,      KO1, \
	KJ1,    KF1, KF0, KA0, KG0, KG1, KH1, KH0, KK0, KI0, KN0,          KJ0, \
	                                                              KN1,      \
	KM1, KC2, KB3, KD1,            KE1,            KD0, KM0, KL6, KI1, KP2  \
) LAYOUT_framework_iso( \
	KF7,    KF3, KF2, KE6, KE3, KK4, KK3, KK2, KP1, KL3, KI4, KI6, KN3,    KB0, \
	KC4,   KC5, KF5, KE5, KG5, KG4, KH4, KH5, KK5, KI5, KN4, KN2, KO4,     KO5, \
	KC3,    KC0, KF6, KE2, KG6, KG3, KH3, KH6, KK6, KI3, KN5, KN6, KO6,         \
	KE4,     KC7, KF4, KO7, KG7, KG2, KH2, KH7, KK7, KI7, KN7, KO0, KI2,   KO1, \
	KJ1, KC_NO, KF1, KF0, KA0, KG0, KG1, KH1, KH0, KK0, KI0, KN0,          KJ0, \
	                                                                  KN1,      \
	KM1, KC2, KB3, KD1,            KE1,                KD0, KM0, KL6, KI1, KP2  \
)

typedef uint8_t layer_state_t;
void layer_state_set(layer_state_t state);
void layer_on(uint8_t layer);
void layer_off(uint8_t layer);
void layer_invert(uint8_t layer);
bool layer_state_cmp(layer_state_t state, uint8_t layer);
bool layer_state_is(uint8_t layer);

/* HOOKS */
uint8_t layer_state_set_kb(uint8_t state);
uint8_t layer_state_set_user(uint8_t state);

extern const uint16_t keymaps[][KEYBOARD_COLS_MAX][KEYBOARD_ROWS];

struct key_pos {
	uint8_t row;
	uint8_t col;
};

struct key_event {
	struct key_pos key;
	bool pressed;
};

struct key_tap {
	uint8_t count;
};

typedef struct key_record {
	struct key_event event;
	struct key_tap tap;
} keyrecord_t; // QMK compatibility name

// tap.count is 1 when the tap event fired
// tap.count is 0 when the hold event fired

bool process_record_user(uint16_t keycode, keyrecord_t* record);
bool process_record_kb(uint16_t keycode, keyrecord_t* record);
bool process_record_proto(uint16_t keycode, keyrecord_t* record);
bool process_record(uint16_t keycode, keyrecord_t* record);

// Suspend and Resume hooks are called when the computer goes into sleep
// and resumes from S3 or S0ix. They are also called during system boot and
// shutdown, as the system transitions through all power states on the way up
// or down.
//
// They will typically be used to toggle the states of LEDs.
void ko_suspend_kb(void);
void ko_suspend_user(void);
void ko_resume_kb(void);
void ko_resume_user(void);
