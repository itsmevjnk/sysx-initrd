#ifndef KBD_SCSET2_H
#define KBD_SCSET2_H

#include <stddef.h>
#include <stdint.h>
#include <hal/kbdcodes.h>

/* PS/2 keyboard scancode set 2 lookup tables */

const uint8_t ps2_kbd_scset2_reg[] = {
    KEY_NONE,           KEY_F9,             KEY_NONE,           KEY_F5,
    KEY_F3,             KEY_F1,             KEY_F2,             KEY_F12,
    KEY_NONE,           KEY_F10,            KEY_F8,             KEY_F6,
    KEY_F4,             KEY_TAB,            KEY_GRAVE,          KEY_NONE,
    KEY_NONE,           KEY_LEFTALT,        KEY_LEFTSHIFT,      KEY_NONE,
    KEY_LEFTCTRL,       KEY_Q,              KEY_1,              KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_Z,              KEY_S,
    KEY_A,              KEY_W,              KEY_2,              KEY_NONE,
    KEY_NONE,           KEY_C,              KEY_X,              KEY_D,
    KEY_E,              KEY_4,              KEY_3,              KEY_NONE,
    KEY_NONE,           KEY_SPACE,          KEY_V,              KEY_F,
    KEY_T,              KEY_R,              KEY_5,              KEY_NONE,
    KEY_NONE,           KEY_N,              KEY_B,              KEY_H,
    KEY_G,              KEY_Y,              KEY_6,              KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_M,              KEY_J,
    KEY_U,              KEY_7,              KEY_8,              KEY_NONE,
    KEY_NONE,           KEY_COMMA,          KEY_K,              KEY_I,
    KEY_O,              KEY_0,              KEY_9,              KEY_NONE,
    KEY_NONE,           KEY_DOT,            KEY_SLASH,          KEY_L,
    KEY_SEMICOLON,      KEY_P,              KEY_MINUS,          KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_APOSTROPHE,     KEY_NONE,
    KEY_LEFTBRACE,      KEY_EQUAL,          KEY_NONE,           KEY_NONE,
    KEY_CAPSLOCK,       KEY_RIGHTSHIFT,     KEY_ENTER,          KEY_RIGHTBRACE,
    KEY_NONE,           KEY_BACKSLASH,      KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_BACKSPACE,      KEY_NONE,
    KEY_NONE,           KEY_KP1,            KEY_NONE,           KEY_KP4,
    KEY_KP7,            KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_KP0,            KEY_KPDOT,          KEY_KP2,            KEY_KP5,
    KEY_KP6,            KEY_KP8,            KEY_ESC,            KEY_NUMLOCK,
    KEY_F11,            KEY_KPPLUS,         KEY_KP3,            KEY_KPMINUS,
    KEY_KPASTERISK,     KEY_KP9,            KEY_SCROLLLOCK,     KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_F7
};

const uint8_t ps2_kbd_scset2_ext[] = {
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_MEDIA_FIND,     KEY_RIGHTALT,       KEY_NONE,           KEY_NONE,
    KEY_RIGHTCTRL,      KEY_MEDIA_PREVIOUSSONG, KEY_NONE,       KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_LEFTMETA,
    KEY_MEDIA_REFRESH,  KEY_MEDIA_VOLUMEDOWN, KEY_NONE,         KEY_MEDIA_MUTE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_RIGHTMETA,
    KEY_MEDIA_STOP,     KEY_NONE,           KEY_NONE,           KEY_MEDIA_CALC,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_COMPOSE,
    KEY_MEDIA_FORWARD,  KEY_NONE,           KEY_MEDIA_VOLUMEUP, KEY_NONE,
    KEY_MEDIA_PLAYPAUSE,KEY_NONE,           KEY_NONE,           KEY_POWER,
    KEY_MEDIA_BACK,     KEY_NONE,           KEY_MEDIA_WWW,      KEY_MEDIA_STOPCD,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_KPSLASH,        KEY_NONE,
    KEY_NONE,           KEY_MEDIA_NEXTSONG, KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_KPENTER,        KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_END,            KEY_NONE,           KEY_LEFT,
    KEY_HOME,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_INSERT,         KEY_DELETE,         KEY_DOWN,           KEY_NONE,
    KEY_RIGHT,          KEY_UP,             KEY_NONE,           KEY_PAUSE,
    KEY_NONE,           KEY_NONE,           KEY_PAGEDOWN,       KEY_NONE,
    KEY_SYSRQ,          KEY_PAGEUP
};

#endif