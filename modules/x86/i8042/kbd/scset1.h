#ifndef KBD_SCSET1_H
#define KBD_SCSET1_H

#include <stddef.h>
#include <stdint.h>
#include <hal/kbdcodes.h>

/* PS/2 keyboard scancode set 1 lookup tables */

const uint8_t ps2_kbd_scset1_reg[] = {
    KEY_NONE,           KEY_ESC,            KEY_1,              KEY_2,
    KEY_3,              KEY_4,              KEY_5,              KEY_6,
    KEY_7,              KEY_8,              KEY_9,              KEY_0,
    KEY_MINUS,          KEY_EQUAL,          KEY_BACKSPACE,      KEY_TAB,
    KEY_Q,              KEY_W,              KEY_E,              KEY_R,  
    KEY_T,              KEY_Y,              KEY_U,              KEY_I,
    KEY_O,              KEY_P,              KEY_LEFTBRACE,      KEY_RIGHTBRACE,
    KEY_ENTER,          KEY_LEFTCTRL,       KEY_A,              KEY_S,
    KEY_D,              KEY_F,              KEY_G,              KEY_H,  
    KEY_J,              KEY_K,              KEY_L,              KEY_SEMICOLON,
    KEY_APOSTROPHE,     KEY_GRAVE,          KEY_LEFTSHIFT,      KEY_BACKSLASH,
    KEY_Z,              KEY_X,              KEY_C,              KEY_V,
    KEY_B,              KEY_N,              KEY_M,              KEY_COMMA,
    KEY_DOT,            KEY_SLASH,          KEY_RIGHTSHIFT,     KEY_KPASTERISK,
    KEY_LEFTALT,        KEY_SPACE,          KEY_CAPSLOCK,       KEY_F1,
    KEY_F2,             KEY_F3,             KEY_F4,             KEY_F5,
    KEY_F6,             KEY_F7,             KEY_F8,             KEY_F9,
    KEY_F10,            KEY_NUMLOCK,        KEY_SCROLLLOCK,     KEY_KP7,
    KEY_KP8,            KEY_KP9,            KEY_KPMINUS,        KEY_KP4,
    KEY_KP5,            KEY_KP6,            KEY_KPPLUS,         KEY_KP1,
    KEY_KP2,            KEY_KP3,            KEY_KP0,            KEY_KPDOT,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_F11,
    KEY_F12
};

const uint8_t ps2_kbd_scset1_ext[] = {
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_MEDIA_PREVIOUSSONG, KEY_NONE,       KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_MEDIA_NEXTSONG, KEY_NONE,           KEY_NONE,
    KEY_KPENTER,        KEY_RIGHTCTRL,      KEY_NONE,           KEY_NONE,
    KEY_MEDIA_MUTE,     KEY_MEDIA_CALC,     KEY_MEDIA_PLAYPAUSE,KEY_NONE,
    KEY_MEDIA_STOPCD,   KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_MEDIA_VOLUMEDOWN, KEY_NONE,
    KEY_MEDIA_VOLUMEUP, KEY_NONE,           KEY_MEDIA_WWW,      KEY_NONE,
    KEY_NONE,           KEY_KPSLASH,        KEY_NONE,           KEY_SYSRQ, // NOTE: PrtSc/SysRq is E0 2A E0 37
    KEY_RIGHTALT,       KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_HOME,
    KEY_UP,             KEY_PAGEUP,         KEY_NONE,           KEY_LEFT,
    KEY_NONE,           KEY_RIGHT,          KEY_NONE,           KEY_END,
    KEY_DOWN,           KEY_PAGEDOWN,       KEY_INSERT,         KEY_DELETE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_LEFTMETA,
    KEY_RIGHTMETA,      KEY_COMPOSE,        KEY_POWER,          KEY_NONE,
    KEY_NONE,           KEY_NONE,           KEY_NONE,           KEY_NONE,
    KEY_NONE,           KEY_MEDIA_FIND,     KEY_NONE,           KEY_MEDIA_REFRESH,
    KEY_MEDIA_STOP,     KEY_MEDIA_FORWARD,  KEY_MEDIA_BACK,     KEY_NONE,
    KEY_NONE,           KEY_NONE
};

#endif