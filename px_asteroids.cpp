
// export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
// PATH=/System/Volumes/Data/w/external/clang+llvm-13.0.0-x86_64-apple-darwin/bin/:$PATH
// function compile() { clang -glldb -Werror -Wduplicate-enum -Wshadow-all -fsanitize=address -F /Library/Frameworks/ -c $1.cpp -MJ build/$1.o.compdb -o build/$1.o; }
// compile px_asteroids
// clang -gembed-source -gfull -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec
// compile px_asteroids; compile soft_window; clang -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec


#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "soft_window.h"

template<class T> T * recast(void * memory, s64 offset = 0) { return (T*)((u8*)memory + offset); }


struct Pixel { u8 red, grn, blu, alf; };
static inline Pixel * get_pixel(s16 px_x, s16 px_y) {
    if (px_x >= 0 && px_x < SoftWindow::width && px_y >= 0 && px_y < SoftWindow::height) {
        return recast<Pixel>(SoftWindow::buffer, px_y * SoftWindow::pitch + px_x * 4);
    } else {
        return 0;
    }
}

static inline Pixel * set_pixel(s16 px_x, s16 px_y, Pixel px) {
    auto pxp = get_pixel(px_x, px_y);
    if (pxp) { *pxp = px; }
    return pxp;
}

static inline void draw_box(s32 l, s32 t, s32 w, s32 h, Pixel color) {
    for (int box_px_y = t; box_px_y < t + h; box_px_y++) {
        for (int box_px_x = l; box_px_x < l + w; box_px_x++) {
            set_pixel(box_px_x, box_px_y, color);
        }
    }
}

static inline void draw_line(s16 x0, s16 y0, s16 x1, s16 y1, Pixel color) {
    auto dx =  abs(x1-x0);
    auto sx = x0 < x1 ? 1 : -1;
    auto dy = -abs(y1-y0);
    auto sy = y0 < y1 ? 1 : -1;
    auto err = dx + dy;
    set_pixel(x0, y0, color);
    while (x0 != x1 || y0 != y1) {
        auto e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
        set_pixel(x0, y0, color);
    }
}

static Pixel white = {0xff, 0xff, 0xff, 0xff};
static Pixel megenta = {0xff, 0x5f, 0xff, 0xff};

struct AffineTransform {
    float a, b, c,
          d, e, f,
          g, h, i;
    // void zero() { *this = {}; }
    // void identity() { zero(); a = e = i = 1; }
    // void translate(float x, float y) { identity(); c=x; f=y; }
    // void scale(float s) { identity(); a = e =s; }
    // void rotate(float t) { zero(); a = e = cosf(t); b = -(d = sinf(t)); i = 1; }

    void _mul(AffineTransform o) {
        auto & t = *this;
        *this = {
            o.a*t.a + o.b*t.d + o.c*t.g,
            o.a*t.b + o.b*t.e + o.c*t.h,
            o.a*t.c + o.b*t.f + o.c*t.i,

            o.d*t.a + o.e*t.d + o.f*t.g,
            o.d*t.b + o.e*t.e + o.f*t.h,
            o.d*t.c + o.e*t.f + o.f*t.i,

            o.g*t.a + o.h*t.d + o.i*t.g,
            o.g*t.b + o.h*t.e + o.i*t.h,
            o.g*t.c + o.h*t.f + o.i*t.i,
        };
    }
    static auto zero() { return AffineTransform{}; }
    static auto identity() { auto r=zero(); r.a=r.e=r.i=1; return r;}
    void translate(float x, float y) { auto o=identity(); o.c=x; o.f=y; _mul(o); }
    void scale(float s) { auto o=identity(); o.a=o.e=s;  _mul(o); }
    void rotate(float t) { auto o=zero(); o.a=o.e=cosf(t); o.b = -(o.d = sinf(t)); o.i = 1;  _mul(o); }
};


struct Vector { s16 x, y; };
Vector vec_project_point(Vector v, AffineTransform tf) {
    return {
        s16(v.x * tf.a + v.y * tf.b + 1.0f * tf.c),
        s16(v.x * tf.d + v.y * tf.e + 1.0f * tf.f),
    };
}

static inline void draw_line(Vector v0, Vector v1, Pixel color) {
    draw_line(v0.x, v0.y, v1.x, v1.y, color);
}






int main() {
    SoftWindow::init();
    int should_keep_running = 1;
    int box_x = 50;
    int box_y = 50;
    int box_r = 0;

    while (should_keep_running) {
        using namespace SoftWindow;
        draw_box(0, 0, width, height, megenta);
        //draw_box(box_x, box_y, 10, 10, white);
        draw_line(200, 200, box_x, box_y, white);

        s8 xs[] = { -10,   0,  10, -10};
        s8 ys[] = { -10,  10, -10, -10};

        for (int i = 1; i < 4; i++) {
            auto box_m = AffineTransform::identity();
            box_m.rotate(box_r / 1000.0f);
            box_m.translate(box_x, box_y);
            draw_line(
                vec_project_point({xs[i-1], ys[i-1]}, box_m),
                vec_project_point({xs[i-0], ys[i-0]}, box_m),
                white);
        }

        SoftWindow::update();
        //printf("delta_ms:%d\n", delta_ms);
        if (SoftWindow::key_presses[KEY_QUIT]) {
            should_keep_running = 0;
        }
        AffineTransform new_m = AffineTransform::identity();
        if (key_downs[ KEY_LEFT  ]) { box_x--; }
        if (key_downs[ KEY_RIGHT ]) { box_x++; }
        if (key_downs[ KEY_UP    ]) { box_r-=10; }
        if (key_downs[ KEY_DOWN  ]) { box_r+=10; }
    }
}
