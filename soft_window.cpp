#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "soft_window.h"

namespace SoftWindow {
    Pixel * pixels;
    u16 width, height, mouse_x, mouse_y;
    s16 mouse_dx, mouse_dy;
    u32 pitch;
    u8 key_presses[256];
    u8 key_downs[256];
    u8 delta_ms;


    static SDL_Window * sdl_window;
    static u16 keymap[SDL_NUM_SCANCODES];

    void _keymap_from_literal(char c) {
        char name[2] = "a"; name[0] = c;
        keymap[SDL_GetScancodeFromName(name)] = c;
    }

    static void _init_surface() {
        auto surface = SDL_GetWindowSurface(sdl_window);
        assert(surface->format->BytesPerPixel == 4);
        assert(!SDL_MUSTLOCK(surface));

        width = surface->w;
        height = surface->h;
        pitch = surface->pitch / sizeof(Pixel);
        assert(surface->pitch % sizeof(Pixel) == 0);
        pixels = (Pixel*)surface->pixels;
    }

    void init() {
        if (sdl_window) {
            SDL_DestroyWindow(sdl_window);
        } else {
            SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
            SDL_Init(SDL_INIT_VIDEO);

            keymap[SDL_SCANCODE_LALT] = KEY_LEFT_ALT;
            keymap[SDL_SCANCODE_RALT] = KEY_RIGHT_ALT;
            keymap[SDL_SCANCODE_LSHIFT] = KEY_LEFT_SHIFT;
            keymap[SDL_SCANCODE_RSHIFT] = KEY_RIGHT_SHIFT;
            keymap[SDL_SCANCODE_LCTRL] = KEY_LEFT_CTRL;
            keymap[SDL_SCANCODE_RCTRL] = KEY_RIGHT_CTRL;
            // KEY_QUIT, not a scancode
            keymap[SDL_SCANCODE_BACKSPACE] = KEY_BACKSPACE;
            keymap[SDL_SCANCODE_F1] = KEY_F1;
            keymap[SDL_SCANCODE_F2] = KEY_F2;
            keymap[SDL_SCANCODE_F3] = KEY_F3;
            keymap[SDL_SCANCODE_F4] = KEY_F4;
            keymap[SDL_SCANCODE_F5] = KEY_F5;
            keymap[SDL_SCANCODE_F6] = KEY_F6;
            keymap[SDL_SCANCODE_F7] = KEY_F7;
            keymap[SDL_SCANCODE_F8] = KEY_F8;
            keymap[SDL_SCANCODE_F9] = KEY_F9;
            keymap[SDL_SCANCODE_F10] = KEY_F10;
            keymap[SDL_SCANCODE_F11] = KEY_F11;
            keymap[SDL_SCANCODE_F12] = KEY_F12;
            keymap[SDL_SCANCODE_F13] = KEY_F13;
            keymap[SDL_SCANCODE_F14] = KEY_F14;

            keymap[SDL_SCANCODE_LGUI] = KEY_LEFT_OS;
            keymap[SDL_SCANCODE_RGUI] = KEY_RIGHT_OS;
            keymap[SDL_SCANCODE_LEFT] = KEY_LEFT;
            keymap[SDL_SCANCODE_RIGHT] = KEY_RIGHT;
            keymap[SDL_SCANCODE_UP] = KEY_UP;
            keymap[SDL_SCANCODE_DOWN] = KEY_DOWN;
            keymap[SDL_SCANCODE_ESCAPE] = KEY_ESCAPE;
            keymap[SDL_SCANCODE_TAB] = KEY_TAB;
            keymap[SDL_SCANCODE_RETURN] = KEY_RETURN;
            keymap[SDL_SCANCODE_SPACE] = KEY_SPACE;

            _keymap_from_literal('\'');
            _keymap_from_literal(';');
            for (int i = ','; i <= '9'; i++) _keymap_from_literal(i);
            _keymap_from_literal('=');
            for (int i = 'A'; i <= ']'; i++) _keymap_from_literal(i);
            _keymap_from_literal('`');
            keymap[SDL_SCANCODE_DELETE] = KEY_DELETE;
        }

        sdl_window = SDL_CreateWindow(
            "hello_sdl_pxbuf",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        _init_surface();


    }

    static void _do_keypress(u8 key, int presses = 1) {
        if (key_presses[key] + presses < 250) key_presses[key]+= presses;
        else key_presses[key] = 250;
        key_downs[key] = presses;
    }
    static void _do_keyup(u8 key) { key_downs[key] = 0; }


    void update() {
        if (!sdl_window) init();

        SDL_UpdateWindowSurface( sdl_window );

        memset(key_presses, 0, sizeof key_presses);

        {
            static u64 last_frame_time_ms;
            auto this_frame_time_ms = SDL_GetTicks64();
            if (this_frame_time_ms > last_frame_time_ms) {
                delta_ms =  this_frame_time_ms - last_frame_time_ms;
            } else {
                delta_ms = 5;
            }
            // SDL_Log("frame2: %lld %ld %d\n", this_frame_time_ms, last_frame_time_ms, delta_ms);
            last_frame_time_ms = this_frame_time_ms;
        }

        // TODO: Stereo S16
        // TODO: Mouse Delta, Mouse ABS, Controllers (Axis/Buttons), Mic f32 mono

        SDL_Event evt;
        while(SDL_PollEvent(&evt)) {
            switch (evt.type) {
                case SDL_WINDOWEVENT: {
                    switch (evt.window.event) {
                        case SDL_WINDOWEVENT_RESIZED: { _init_surface(); } break;
                    }
                } break;
                case SDL_QUIT: { _do_keypress(KEY_QUIT); } break;
                case SDL_KEYDOWN: { _do_keypress(keymap[evt.key.keysym.scancode]); } break;
                case SDL_KEYUP: { _do_keyup(keymap[evt.key.keysym.scancode]); } break;
                case SDL_MOUSEMOTION: {
                    if (__builtin_add_overflow(mouse_dx, evt.motion.xrel, &mouse_dx)) {
                        mouse_dx = -1;
                    }
                    if (__builtin_add_overflow(mouse_dy, evt.motion.yrel, &mouse_dy)) {
                        mouse_dy = -1;
                    }
                    mouse_x = evt.motion.x;
                    mouse_y = evt.motion.y;
                } break;
                case SDL_MOUSEBUTTONDOWN: {
                    if (evt.button.button > 16) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Bad mouse button: %d\n", evt.button.button);
                    } else {
                        _do_keypress(KEY_MOUSE_0 + evt.button.button);
                    }
                } break;
                case SDL_MOUSEBUTTONUP: {
                    if (evt.button.button > 16) {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Bad mouse button: %d\n", evt.button.button);
                    } else {
                        _do_keyup(KEY_MOUSE_0 + evt.button.button);
                    }
                } break;
            }
        }
    }

    void set_title(char const * fmt, ...) {
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        SDL_SetWindowTitle(sdl_window, buf);
    }


}