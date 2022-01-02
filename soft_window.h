typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef          char   s8;
typedef          short s16;
typedef          int   s32;
typedef          long  s64;

namespace SoftWindow {
    struct Color { u8 red, grn, blu, alf; } extern * pixels;
    extern u16 width, height, mouse_x, mouse_y;
    extern s16 mouse_dx, mouse_dy;
    extern u32 pitch;
    extern unsigned char key_presses[256];
    extern unsigned char key_downs[256];
    extern u8 delta_ms;

    enum Key { // TODO: add full keyboard
        KEY_INVALID,
        KEY_LEFT_ALT, // also KEY_LEFT_OPTION
        KEY_RIGHT_ALT, // also KEY_RIGHT_OPTION
        KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT,
        KEY_LEFT_CTRL, KEY_RIGHT_CTRL,
        KEY_QUIT,
        KEY_BACKSPACE = 8,
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
        KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
        KEY_F11, KEY_F12, KEY_F13, KEY_F14,
        KEY_LEFT_OS, // also KEY_LEFT_WIN and KEY_LEFT_COMMAND
        KEY_RIGHT_OS, // also KEY_RIGHT_WIN and KEY_RIGHT_COMMAND
        KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
        KEY_ESCAPE,
        KEY_TAB,
        KEY_RETURN, // Also KEY_ENTER
        KEY_SPACE = ' ', // 32, Also literal
        // 33 - 38 Open
        KEY_SINGLE_QUOTE = '\'', // 39, Also literal
        // 40 - 43 Open
        // 44 - 57  Reserved for literals: , - . / 0-9
        // 58 Open
        // 59 Reserved for literal: ;
        // 60 Open
        // 61 Reserved for literal: =
        // 62 - 64 Open
        // 65 - 93 Reserved for literals: A-Z [ \ ]
        // 94-95 Open
        // 96 Reserved for literal: `
        // 97 - 126 Open
        KEY_DELETE = 127,
        KEY_MOUSE_0, KEY_MOUSE_1, KEY_MOUSE_2, KEY_MOUSE_3, KEY_MOUSE_4, KEY_MOUSE_5,
        KEY_MOUSE_6, KEY_MOUSE_7, KEY_MOUSE_8, KEY_MOUSE_9, KEY_MOUSE_10, KEY_MOUSE_11,
        KEY_MOUSE_12, KEY_MOUSE_13, KEY_MOUSE_14, KEY_MOUSE_15,
        // 144 - 255 Open
        KEY_INVALID_OVERFLOW = 256
    };

    enum {  // Convenience aliases
        KEY_LEFT_OPTION = KEY_LEFT_ALT,
        KEY_RIGHT_OPTION = KEY_RIGHT_ALT,
        KEY_LEFT_WIN = KEY_LEFT_OS,
        KEY_LEFT_COMMAND = KEY_LEFT_OS,
        KEY_RIGHT_WIN = KEY_RIGHT_OS,
        KEY_RIGHT_COMMAND = KEY_RIGHT_OS,
        KEY_ENTER = KEY_RETURN,
    };

    void init();
    void update();
    void set_title(char const * fmt, ...); // This is a slow operation

}
