// Platform-specific includes for Keyboard Overdrive
#pragma once
#ifndef __KO_PLATFORM_INCLUDED
#define __KO_PLATFORM_INCLUDED

#include "common.h"
#include "stdbool.h"
#include "system.h"
#include "util.h"
#include "keyboard_overdrive.h"

#define CPUTS(outstr) cputs(CC_KEYBOARD, outstr)
#define CPRINTS(format, args...) cprints(CC_KEYBOARD, format, ## args)
#define CPRINTF(format, args...) cprintf(CC_KEYBOARD, format, ## args)

#define ko_topmost_active_layer(layers) __fls((layers))
void ko_send_keycode(uint16_t keycode, struct key_record* record);
void ko_send_modifiers(uint8_t modifiers, struct key_record* record);
void ko_enqueue_tap_hold_event(uint16_t keycode, struct key_record* record);
bool ko_cancel_tap_hold_event(uint16_t keycode, struct key_record* record);
bool ko_is_enabled(void);

#endif
