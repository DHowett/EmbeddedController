#include <stdint.h>

#define KEYBOARD_COLS_MAX 16
#define KEYBOARD_ROWS 8
#define NUM_LAYERS_MAX 8
#define LAYER_BITS 3

enum _opcode {
	OP_NONE         = 0b0000, // OP 4, MOD 4, KEY 8
	OP_MOD_TAP      = 0b0001, // OP 4, MOD 4, KEY 8
	// Layer only when held
	// Layer lock on
	// Layer lock off
	// Layer Tap (LT) => Layer when held, key when tapped (like mod tap)
	OP_LAYER_TAP    = 0b0010, // OP 4, LAY 4, KEY 8, +press -release
	OP_LAYER_TOGGLE = 0b0011, // OP 4, LAY 4, ___ 8, toggles on release
	OP_LAYER_MASK = 0b0100, // OP 4, ___ 4, LAYERS 8, +press -release of whole layer mask
};

#define ACT(op, val) (op<<12|val)
#define ACT_MOD_TAP(mod, key) ACT(OP_MOD_TAP, ((mod)&0xf)<<8|(key))
#define ACT_LAYER_TAP(lay, key) ACT(OP_LAYER_TAP, ((lay)&0xf)<<8|(key))
#define ACT_LAYER_TOGGLE(lay) ACT(OP_LAYER_TOGGLE, ((lay)&0xf)<<8)
#define KEY_GET_OP(kv) (((kv)>>12)&0xf)
#define KEY_GET_LAYER(kv) (((kv)>>8)&0xf)
#define KEY_GET_LAYER_MASK(kv) ((kv)&0xff)
#define KEY_GET_KC(kv) ((kv)&0xff)
#define KEY_GET_MOD(kv) (((kv)>>8)&0xf)
#define ACT_MOD(mod, key) ACT(OP_NONE, ((mod)&0xf)<<8|(key))

// QMK compatibility
#define MO(layer) ACT_LAYER_TAP(layer, 0)
#define LT(layer, kc) ACT_LAYER_TAP(layer, kc)
#define TG(layer) ACT_LAYER_TOGGLE(layer)
#define MT(mod, kc) ACT_MOD_TAP(mod, kc)

/*
#define KC_NO 0
#define KC_TRANSPARENT 0xFFFF
#define _______ KC_TRANSPARENT

#define KC_0    0x0045
#define KC_1    0x0016
#define KC_2    0x001E
#define KC_3    0x0026
#define KC_4    0x0025
#define KC_5    0x002E
#define KC_6    0x0036
#define KC_7    0x003D
#define KC_8    0x003E
#define KC_9    0x0046
#define KC_A    0x001C
#define KC_B    0x0032
#define KC_C    0x0021
#define KC_D    0x0023
#define KC_E    0x0024
#define KC_F    0x002B
#define KC_G    0x0034
#define KC_H    0x0033
#define KC_I    0x0043
#define KC_J    0x003B
#define KC_K    0x0042
#define KC_L    0x004B
#define KC_M    0x003A
#define KC_N    0x0031
#define KC_O    0x0044
#define KC_P    0x004D
#define KC_Q    0x0015
#define KC_R    0x002D
#define KC_S    0x001B
#define KC_T    0x002C
#define KC_U    0x003C
#define KC_V    0x002A
#define KC_W    0x001D
#define KC_X    0x0022
#define KC_Y    0x0035
#define KC_Z    0x001A
#define KC_BS   0x0066
#define KC_BSLS 0x005D
#define KC_CAPS 0x0058
#define KC_COMM 0x0041
#define KC_DEL  0xE071
#define KC_DOT  0x0049
#define KC_DOWN 0xE072
#define KC_END  0xE069
#define KC_ENT  0x005A
#define KC_EQL  0x0055
#define KC_ESC  0x0076
#define KC_GRV  0x000E
#define KC_HOME 0xE06C
#define KC_LALT 0x0011
#define KC_LBRC 0x0054
#define KC_LCTL 0x0014
#define KC_LEFT 0xE06B
#define KC_LSFT 0x0012
#define KC_LWIN 0xE01F
#define KC_MENU 0xE02F
#define KC_MINS 0x004E
#define KC_NLCK 0x0077
#define KC_NUBS 0x0061
#define KC_QUOT 0x0052
#define KC_RALT 0xE011
#define KC_RBRC 0x005B
#define KC_RCTL 0xE014
#define KC_RGHT 0xE074
#define KC_RSFT 0x0059
#define KC_SCLN 0x004C
#define KC_SLSH 0x004A
#define KC_SPC  0x0029
#define KC_TAB  0x000D
#define KC_UP   0xE075
#define KC_F1   0x0005
#define KC_F2   0x0006
#define KC_F3   0x0004
#define KC_F4   0x000C
#define KC_F5   0x0003
#define KC_F6   0x000B
#define KC_F7   0x0083
#define KC_F8   0x000A
#define KC_F9   0x0001
#define KC_F10  0x0009
#define KC_F11  0x0078
#define KC_F12  0x0007
#define KC_KP_0 0x0070
#define KC_KP_1 0x0069
#define KC_KP_2 0x0072
#define KC_KP_3 0x007A
#define KC_KP_4 0x006B
#define KC_KP_5 0x0073
#define KC_KP_6 0x0074
#define KC_KP_7 0x006C
#define KC_KP_8 0x0075
#define KC_KP_9 0x007D
#define KC_PAST 0x007C
#define KC_PDOT 0x0071
#define KC_PENT 0xE05A
#define KC_PGDN 0xE07A
#define KC_PGUP 0xE07D
#define KC_PINS 0xE070
#define KC_PMNS 0x007B
#define KC_PPLS 0x0079
#define KC_PSLS 0xE04A
*/

enum _keycode {
KC_NO = 0,
KC_TRANSPARENT = 0xFFFF,
KC_A     = 0x1,
KC_B    ,
KC_C    ,
KC_D    ,
KC_E    ,
KC_F    ,
KC_G    ,
KC_H    ,
KC_I    ,
KC_J    ,
KC_K    ,
KC_L    ,
KC_M    ,
KC_N    ,
KC_O    ,
KC_P    ,
KC_Q    ,
KC_R    ,
KC_S    ,
KC_T    ,
KC_U    ,
KC_V    ,
KC_W    ,
KC_X    ,
KC_Y    ,
KC_Z    ,

KC_0    ,
KC_1    ,
KC_2    ,
KC_3    ,
KC_4    ,
KC_5    ,
KC_6    ,
KC_7    ,
KC_8    ,
KC_9    ,

KC_BS   ,
KC_BSLS ,
KC_CAPS ,
KC_COMM ,
KC_INS,
KC_DEL  ,
KC_DOT  ,
KC_DOWN ,
KC_END  ,
KC_ENT  ,
KC_EQL  ,
KC_ESC  ,
KC_GRV  ,
KC_HOME ,
KC_LALT ,
KC_LBRC ,
KC_LCTL ,
KC_LEFT ,
KC_LSFT ,
KC_LWIN ,
KC_MENU ,
KC_MINS ,
KC_NLCK ,
KC_NUBS ,
KC_QUOT ,
KC_RALT ,
KC_RBRC ,
KC_RCTL ,
KC_RGHT ,
KC_RSFT ,
KC_SCLN ,
KC_SLSH ,
KC_SPC  ,
KC_TAB  ,
KC_UP   ,

KC_F1   ,
KC_F2   ,
KC_F3   ,
KC_F4   ,
KC_F5   ,
KC_F6   ,
KC_F7   ,
KC_F8   ,
KC_F9   ,
KC_F10  ,
KC_F11  ,
KC_F12  ,

KC_KP_0 ,
KC_KP_1 ,
KC_KP_2 ,
KC_KP_3 ,
KC_KP_4 ,
KC_KP_5 ,
KC_KP_6 ,
KC_KP_7 ,
KC_KP_8 ,
KC_KP_9 ,
KC_PAST ,
KC_PDOT ,
KC_PENT ,
KC_PGDN ,
KC_PGUP ,
KC_PINS ,
KC_PMNS ,
KC_PPLS ,
KC_PSLS ,

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

SAFE_AREA,
};

#define _______ KC_TRANSPARENT

enum _ext_keycode {
	FK_RFKL = SAFE_AREA, // RF Kill (Airplane Mode)
	FK_PROJ, // Project (Win+P)
	FK_BRNU, // Brightness Up
	FK_BRND, // Brightness Down
	FK_BKLT, // Keyboard Backlight
	FK_FN,
};

/* what ?
#define 0xE016  0xe03c
#define 0xE01A 0xe054
*/

#define KC_MUHENKAN 0x0067
#define KC_KATAKANA_HIRAGANA 0x0013
#define KC_HENKAN            0x0064
#define KC_RO_KANA           0x0051
#define KC_YEN 0x006a

enum _layers {
	_BASE,
	_FN_ANY,
	_FN_PRESSED,
};

// FK_FN is a layer mask tap for both the ANY and PRESSED layers
//#define FK_FN MO(_FN_PRESSED)
#define FK_FLCK TG(_FN_ANY)

// TODO DH: make sure that FK_FLCK doesn't turn FN_ANY *off* because FK_FN turned it *on*


/* ISO
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

#if 0
LAYOUT_framework_ansi(
KC_ESC,   KC_F1, KC_F2,   KC_F3,   KC_F4,  KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,   KC_DEL,
KC_GRV,  KC_1,  KC_2,    KC_3,    KC_4,   KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,    KC_BS,
KC_TAB,   KC_Q,  KC_W,    KC_E,    KC_R,   KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC,  KC_BSLS,
KC_CAPS,   KC_A,  KC_S,    KC_D,    KC_F,   KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,
KC_LSFT,    KC_Z,  KC_X,    KC_C,    KC_V,   KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,                  KC_RSFT,
                                                                                                           KC_UP,
KC_LCTL, KC_FN, KC_LWIN, KC_LALT, KC_SPC,                                       KC_RALT, KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT
)
#endif


#if 0
uint16_t matrix[] = {
//      A        B        C            D        E        F        G        H        I        J        K        L                     M        N        O        P
/* 0 */ KC_C   , KC_DEL , KC_Q       , KC_RALT, KC_PENT, KC_X   , KC_V   , KC_M   , KC_DOT , KC_RSFT, KC_COMM, KC_KATAKANA_HIRAGANA, KC_RCTL, KC_SLSH, KC_QUOT, KC_YEN, 
/* 1 */ KC_PMNS, KC_PINS, KC_KP_0    , KC_LALT, KC_SPC , KC_Z   , KC_B   , KC_N   , KC_DOWN, KC_LSFT, KC_PAST, KC_HENKAN           , KC_LCTL, KC_UP  , KC_ENT , KC_F8, 
/* 2 */ KC_PPLS, KC_KP_9, KC_FN      , 0      , KC_E   , KC_F2  , KC_G   , KC_H   , KC_BSLS, 0      , KC_F7  , KC_KP_8             , 0      , KC_MINS, 0xE016 , KC_RGHT, 
/* 3 */ KC_KP_2, KC_LWIN, KC_TAB     , 0      , KC_F4  , KC_F1  , KC_T   , KC_Y   , KC_O   , 0      , KC_F6  , KC_F9               , 0      , KC_F12 , KC_END , 0xE01A, 
/* 4 */ KC_KP_3, KC_KP_7, KC_GRV     , 0      , KC_CAPS, KC_S   , KC_5   , KC_6   , KC_F10 , 0      , KC_F5  , KC_RO_KANA          , 0      , KC_0   , KC_EQL , 0 , 
/* 5 */ KC_PDOT, KC_HOME, KC_1       , 0      , KC_3   , KC_2   , KC_4   , KC_7   , KC_9   , 0      , KC_8   , KC_102ND            , 0      , KC_P   , KC_BS  , KC_KP_4, 
/* 6 */ KC_KP_1, KC_PGUP, KC_MUHENKAN, 0      , KC_F3  , KC_W   , KC_R   , KC_U   , KC_F11 , 0      , KC_I   , KC_LEFT             , 0      , KC_LBRC, KC_RBRC, KC_KP_5, 
/* 7 */ KC_PSLS, KC_NLCK, KC_A       , 0      , KC_PGDN, KC_ESC , KC_F   , KC_J   , KC_L   , 0      , KC_K   , KC_MENU             , 0      , KC_SCLN, KC_D   , KC_KP_6, 
};
#endif

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
	[KC_0] = 0x45,
	[KC_1] = 0x16,
	[KC_2] = 0x1E,
	[KC_3] = 0x26,
	[KC_4] = 0x25,
	[KC_5] = 0x2E,
	[KC_6] = 0x36,
	[KC_7] = 0x3D,
	[KC_8] = 0x3E,
	[KC_9] = 0x46,
	[KC_A] = 0x1C,
	[KC_B] = 0x32,
	[KC_C] = 0x21,
	[KC_D] = 0x23,
	[KC_E] = 0x24,
	[KC_F] = 0x2B,
	[KC_G] = 0x34,
	[KC_H] = 0x33,
	[KC_I] = 0x43,
	[KC_J] = 0x3B,
	[KC_K] = 0x42,
	[KC_L] = 0x4B,
	[KC_M] = 0x3A,
	[KC_N] = 0x31,
	[KC_O] = 0x44,
	[KC_P] = 0x4D,
	[KC_Q] = 0x15,
	[KC_R] = 0x2D,
	[KC_S] = 0x1B,
	[KC_T] = 0x2C,
	[KC_U] = 0x3C,
	[KC_V] = 0x2A,
	[KC_W] = 0x1D,
	[KC_X] = 0x22,
	[KC_Y] = 0x35,
	[KC_Z] = 0x1A,
	[KC_BS] = 0x66,
	[KC_BSLS] = 0x5D,
	[KC_CAPS] = 0x58,
	[KC_COMM] = 0x41,
	[KC_INS] = _E0(0x70),
	[KC_DEL] = _E0(0x71),
	[KC_DOT] = 0x49,
	[KC_DOWN] = _E0(0x72),
	[KC_END] = _E0(0x69),
	[KC_ENT] = 0x5A,
	[KC_EQL] = 0x55,
	[KC_ESC] = 0x76,
	[KC_GRV] = 0x0E,
	[KC_HOME] = _E0(0x6C),
	[KC_LALT] = 0x11,
	[KC_LBRC] = 0x54,
	[KC_LCTL] = 0x14,
	[KC_LEFT] = _E0(0x6B),
	[KC_LSFT] = 0x12,
	[KC_LWIN] = _E0(0x1F),
	[KC_MENU] = _E0(0x2F),
	[KC_MINS] = 0x4E,
	[KC_NLCK] = 0x77,
	[KC_NUBS] = 0x61,
	[KC_QUOT] = 0x52,
	[KC_RALT] = _E0(0x11),
	[KC_RBRC] = 0x5B,
	[KC_RCTL] = _E0(0x14),
	[KC_RGHT] = _E0(0x74),
	[KC_RSFT] = 0x59,
	[KC_SCLN] = 0x4C,
	[KC_SLSH] = 0x4A,
	[KC_SPC] = 0x29,
	[KC_TAB] = 0x0D,
	[KC_UP] = _E0(0x75),
	[KC_F1] = 0x05,
	[KC_F2] = 0x06,
	[KC_F3] = 0x04,
	[KC_F4] = 0x0C,
	[KC_F5] = 0x03,
	[KC_F6] = 0x0B,
	[KC_F7] = 0x83, // Special case me
	[KC_F8] = 0x0A,
	[KC_F9] = 0x01,
	[KC_F10] = 0x09,
	[KC_F11] = 0x78,
	[KC_F12] = 0x07,
	[KC_KP_0] = 0x70,
	[KC_KP_1] = 0x69,
	[KC_KP_2] = 0x72,
	[KC_KP_3] = 0x7A,
	[KC_KP_4] = 0x6B,
	[KC_KP_5] = 0x73,
	[KC_KP_6] = 0x74,
	[KC_KP_7] = 0x6C,
	[KC_KP_8] = 0x75,
	[KC_KP_9] = 0x7D,
	[KC_PAST] = 0x7C,
	[KC_PDOT] = 0x71,
	[KC_PENT] = _E0(0x5A),
	[KC_PGDN] = _E0(0x7A),
	[KC_PGUP] = _E0(0x7D),
	[KC_PINS] = _E0(0x70),
	[KC_PMNS] = 0x7B,
	[KC_PPLS] = 0x79,
	[KC_PSLS] = _E0(0x4A),
	[KC_MSEL] = _E0(0x50),
	[KC_MNXT] = _E0(0x4D),
	[KC_MPRV] = _E0(0x15),
	[KC_MPLY] = _E0(0x34),
	[KC_VOLD] = _E0(0x21),
	[KC_VOLU] = _E0(0x32),
	[KC_MUTE] = _E0(0x23),
	[KC_PSCR] = 0xFC, // E0-7C PrintScr - Might need another special key (E0 12)
	[KC_CTBR] = 0xFE, // E0-7E Ctrl+Break
	[KC_PAUS] = 0xFF, // E1-14-77-E1-F0-14-F0-77 Pause
};
#undef _E0


const char* keyNames[] ={
[KC_NO] = "KC_NO",
[KC_TRANSPARENT] = "KC_TRANSPARENT",
[KC_A] = "KC_A",
[KC_B] = "KC_B",
[KC_C] = "KC_C",
[KC_D] = "KC_D",
[KC_E] = "KC_E",
[KC_F] = "KC_F",
[KC_G] = "KC_G",
[KC_H] = "KC_H",
[KC_I] = "KC_I",
[KC_J] = "KC_J",
[KC_K] = "KC_K",
[KC_L] = "KC_L",
[KC_M] = "KC_M",
[KC_N] = "KC_N",
[KC_O] = "KC_O",
[KC_P] = "KC_P",
[KC_Q] = "KC_Q",
[KC_R] = "KC_R",
[KC_S] = "KC_S",
[KC_T] = "KC_T",
[KC_U] = "KC_U",
[KC_V] = "KC_V",
[KC_W] = "KC_W",
[KC_X] = "KC_X",
[KC_Y] = "KC_Y",
[KC_Z] = "KC_Z",

[KC_0] = "KC_0",
[KC_1] = "KC_1",
[KC_2] = "KC_2",
[KC_3] = "KC_3",
[KC_4] = "KC_4",
[KC_5] = "KC_5",
[KC_6] = "KC_6",
[KC_7] = "KC_7",
[KC_8] = "KC_8",
[KC_9] = "KC_9",

[KC_BS] = "KC_BS",
[KC_BSLS] = "KC_BSLS",
[KC_CAPS] = "KC_CAPS",
[KC_COMM] = "KC_COMM",
[KC_INS] = "KC_INS",
[KC_DEL] = "KC_DEL",
[KC_DOT] = "KC_DOT",
[KC_DOWN] = "KC_DOWN",
[KC_END] = "KC_END",
[KC_ENT] = "KC_ENT",
[KC_EQL] = "KC_EQL",
[KC_ESC] = "KC_ESC",
[KC_GRV] = "KC_GRV",
[KC_HOME] = "KC_HOME",
[KC_LALT] = "KC_LALT",
[KC_LBRC] = "KC_LBRC",
[KC_LCTL] = "KC_LCTL",
[KC_LEFT] = "KC_LEFT",
[KC_LSFT] = "KC_LSFT",
[KC_LWIN] = "KC_LWIN",
[KC_MENU] = "KC_MENU",
[KC_MINS] = "KC_MINS",
[KC_NLCK] = "KC_NLCK",
[KC_NUBS] = "KC_NUBS",
[KC_QUOT] = "KC_QUOT",
[KC_RALT] = "KC_RALT",
[KC_RBRC] = "KC_RBRC",
[KC_RCTL] = "KC_RCTL",
[KC_RGHT] = "KC_RGHT",
[KC_RSFT] = "KC_RSFT",
[KC_SCLN] = "KC_SCLN",
[KC_SLSH] = "KC_SLSH",
[KC_SPC] = "KC_SPC",
[KC_TAB] = "KC_TAB",
[KC_UP] = "KC_UP",

[KC_F1] = "KC_F1",
[KC_F2] = "KC_F2",
[KC_F3] = "KC_F3",
[KC_F4] = "KC_F4",
[KC_F5] = "KC_F5",
[KC_F6] = "KC_F6",
[KC_F7] = "KC_F7",
[KC_F8] = "KC_F8",
[KC_F9] = "KC_F9",
[KC_F10] = "KC_F10",
[KC_F11] = "KC_F11",
[KC_F12] = "KC_F12",

[KC_KP_0] = "KC_KP_0",
[KC_KP_1] = "KC_KP_1",
[KC_KP_2] = "KC_KP_2",
[KC_KP_3] = "KC_KP_3",
[KC_KP_4] = "KC_KP_4",
[KC_KP_5] = "KC_KP_5",
[KC_KP_6] = "KC_KP_6",
[KC_KP_7] = "KC_KP_7",
[KC_KP_8] = "KC_KP_8",
[KC_KP_9] = "KC_KP_9",
[KC_PAST] = "KC_PAST",
[KC_PDOT] = "KC_PDOT",
[KC_PENT] = "KC_PENT",
[KC_PGDN] = "KC_PGDN",
[KC_PGUP] = "KC_PGUP",
[KC_PINS] = "KC_PINS",
[KC_PMNS] = "KC_PMNS",
[KC_PPLS] = "KC_PPLS",
[KC_PSLS] = "KC_PSLS",

[KC_MSEL] = "KC_MSEL",
[KC_MNXT] = "KC_MNXT",
[KC_MPRV] = "KC_MPRV",
[KC_MPLY] = "KC_MPLY",
[KC_VOLD] = "KC_VOLD",
[KC_VOLU] = "KC_VOLU",
[KC_MUTE] = "KC_MUTE",

[KC_PAUS] = "KC_PAUS",
[KC_CTBR] = "KC_CTBR",
[KC_PSCR] = "KC_PSCR",
[KC_SLCK] = "KC_SLCK",

[SAFE_AREA] = "SAFE_AREA",
};

/*
you can fit 8 lIDs in 24 bits (3 bytes)
each keypos needs 1 lID
128 keypos'
How do we map keypos to lIDpos?
8x16 (RxC)
keymap is stored in Column-major order
Column is looked up first
therefore,
lID layout:
C1 C1 C1
C2 C2 C2
C3 C3 C3 ...

     R  R   R  R  R   R  R  R
A:|00011122 23334445 55666777
B: 00011122|23334445 55666777
C: 00011122 23334445|55666777
D: 00011122 23334445 55666777|
E:|00011122 23334445 55666777
F: 00011122|23334445 55666777
G: 00011122 23334445|55666777
H: 00011122 23334445 55666777|

How do I bit-stride this? it's a uint32_t with only 24 bits filled. 0x7<<(3*(7-r)) is the mask
>>3*(7-r) & 0x7 is the easy way if we have unaligned reads
if we don't, we will end up reading C1 C1 C1 C2 and need to account for the 8 bits in C2.
How do I generalize the algorithm...

uint32_t* rlIDp = 

this code might have to be GPLed.
It looks like TMK uses one byte per column per *layer bit*. So...
C1L1 C1L2 C1L3
C2L1 C2L2 C2L3
C3L1 C3L2 C3L3
   01234567 01234567 01234567
A: 10100000 10000000 10100000 = KA0 had layer bits 0b111, KA1 had layer bits 0b000, KA2 had layer bits 0b101
B: 00000000 10000000 10000000 = KB0 had layer bits 0b110
*/

/*
// lval = 1 bit = 0 sto = 0
apl[row][bit] ^= ( // say this is 10101010
			-(
				(lval & (1U << bit)) != 0 // layer contains bit number: 00000001; lacks: 00000000
			)                                 // layer contains bit number: 11111111; lacks: 00000000 // clever
			^ apl[row][bit]                   // xor with existing contents (why)
		)
	& (1U << sto);

// setting
NEG 00000001
  = 11111111 << bitfield representing "turning on all bits"
XOR 10101010 << original value
  = 01010101 << bitfield of all bits that are out of sync with "all on" = out of sync set
AND 00000001 << narrow it down to the one we care about = out of sync bit
  = 00000001
XOR 10101010 << original value - xor will toggle state of out of sync bit
  = 10101011 << final value with layer bit set

// clearing
NEG 00000000
  = 00000000
XOR 10101010
  = 10101010
AND 00000001
  = 00000000
XOR 10101010
  = 10101010 << final value with layer bit clear

negate is to flood bit field
first xor is to
and is to select correct bit from xor field for storage position
second xor is to lock that bit in?
*/
