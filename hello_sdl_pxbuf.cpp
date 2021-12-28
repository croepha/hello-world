
// export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
// PATH=/System/Volumes/Data/w/external/clang+llvm-13.0.0-x86_64-apple-darwin/bin/:$PATH
// clang -fsanitize=address -v -MJ asdf.json -F /Library/Frameworks/ -framework SDL2 hello_sdl_pxbuf.cpp -o build/hello_sdl_pxbug.exec

#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>
int main(int argc, char* args[]) {

    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow(
        "hello_sdl_pxbuf",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480, SDL_WINDOW_SHOWN
    );
    SDL_Surface * surface = SDL_GetWindowSurface(window);
    assert(surface->format->BytesPerPixel == 4);
    assert(!SDL_MUSTLOCK(surface));

    int should_keep_running = 1;

    int box_x = 50;
    int box_y = 50;

    while (should_keep_running) {

        // fill the whole screen with megenta
        for (int y = 0; y < surface->h; y++) {
            for (int x = 0; x < surface->w; x++) {
                auto pixel = (unsigned char *)surface->pixels + y * surface->pitch + x * 4;
                pixel[0] = 0xff; // Red
                pixel[1] = 0x5f; // Green
                pixel[2] = 0xff; // Blue
                pixel[3] = 0xff; // Alpha
            }
        }

        // Draw a white box at box_x, box_y that is 10px x 10px
        for (int box_px_y = 0; box_px_y < 10; box_px_y++) {
            for (int box_px_x = 0; box_px_x < 10; box_px_x++) {
                auto px_x = box_x + box_px_x;
                auto px_y = box_y + box_px_y;

                auto pixel = (unsigned char *)surface->pixels + px_y * surface->pitch + px_x * 4;
                if (px_x >= 0 && px_x < surface->w && px_y >= 0 && px_y < surface->h) {
                    pixel[0] = 0xff;
                    pixel[1] = 0xff;
                    pixel[2] = 0xff;
                    pixel[3] = 0xff;
                }
            }
        }

        SDL_UpdateWindowSurface( window );

        // Handle input
        SDL_Event e;
        while( SDL_PollEvent( &e ) != 0 )
        {
            switch (e.type) {
                case SDL_QUIT: {
                    should_keep_running = false;
                } break;
            }
        }

        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_RIGHT]) box_x++;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_LEFT ]) box_x--;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_DOWN ]) box_y++;
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_UP   ]) box_y--;

    }

}
