typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef          char   s8;
typedef          short s16;
typedef          int   s32;
typedef          long  s64;

namespace SoftWindow {

    extern void * buffer;
    extern u16 width;
    extern u16 height;
    extern u32 pitch;
    extern unsigned char key_presses[256];
    extern unsigned char key_downs[256];
    extern u8 delta_ms;


    enum Key {
        KEY_INVALID,
        KEY_ALT,
        KEY_SHIFT,
        KEY_CTRL,
        KEY_OS,
        KEY_QUIT,
        KET_BACKSPACE = 8,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,

        KEY_DOWN,
        KEY_ESCAPE,
        KEY_SPACE = ' ',
        // anything between here is forbidden, unless it matches ascii/utf8
        KEY_DEL = 127,
        KEY_INVALID_OVERFLOW = 256
    };

    void init();
    void update();

}
