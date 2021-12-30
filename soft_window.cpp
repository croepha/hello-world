// compile soft_window
#include <assert.h>

#include <SDL2/SDL.h>
#include <cstdio>

#include "soft_window.h"

namespace SoftWindow {
    void * buffer;
    u16 width;
    u16 height;
    u32 pitch;
    u8 key_presses[256];
    u8 key_downs[256];
    u8 delta_ms;


    static SDL_Window * sdl_window;
    static u16 keymap[SDL_NUM_SCANCODES];

    void init() {
        SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
        SDL_Init(SDL_INIT_VIDEO);
        sdl_window = SDL_CreateWindow(
            "hello_sdl_pxbuf",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            640, 480, SDL_WINDOW_SHOWN
        );
        auto surface = SDL_GetWindowSurface(sdl_window);
        assert(surface->format->BytesPerPixel == 4);
        assert(!SDL_MUSTLOCK(surface));

        width = surface->w;
        height = surface->h;
        pitch = surface->pitch;
        buffer = surface->pixels;

        for (int i = 32; i < 127; i++) {
            char name[2] = "a";
            name[0] = i;
            keymap[i] = SDL_GetKeyFromName(name);
            //printf("keymap: %03d:%s -> %d\n", i, name, keymap[i]);
        }
        keymap[SDL_GetScancodeFromName( "LEFT"   )] = KEY_LEFT;
        keymap[SDL_GetScancodeFromName( "RIGHT"  )] = KEY_RIGHT;
        keymap[SDL_GetScancodeFromName( "UP"     )] = KEY_UP;
        keymap[SDL_GetScancodeFromName( "DOWN"   )] = KEY_DOWN;
        keymap[SDL_GetScancodeFromName( "ESCAPE" )] = KEY_ESCAPE;

    }

    static void _do_keypress(u8 key) {
        if (key_presses[key] < 250) {
            key_presses[key]++;
        }
        key_downs[key] = 1;
    }
    static void _do_keyup(u8 key) {
        key_downs[key] = 0;
    }


    static u64 last_frame_time_ms;
    void update() {
        SDL_UpdateWindowSurface( sdl_window );

        memset(key_presses, 0, sizeof key_presses);

        {
            auto this_frame_time_ms = SDL_GetTicks64();
            if (this_frame_time_ms > last_frame_time_ms) {
                delta_ms =  this_frame_time_ms - last_frame_time_ms;
            } else {
                delta_ms = 5;
            }
            // SDL_Log("frame2: %lld %ld %d\n", this_frame_time_ms, last_frame_time_ms, delta_ms);
            last_frame_time_ms = this_frame_time_ms;
        }


        // Handle input
        SDL_Event e;
        while( SDL_PollEvent( &e ) != 0 )
        {
            switch (e.type) {
                case SDL_QUIT: {
                    _do_keypress(KEY_QUIT);
                } break;
                case SDL_KEYDOWN: {
                    _do_keypress(keymap[e.key.keysym.scancode]);
                } break;
                case SDL_KEYUP: {
                    _do_keyup(keymap[e.key.keysym.scancode]);
                } break;
            }
        }
    }

}