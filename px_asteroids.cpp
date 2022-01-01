
// export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
// PATH=/System/Volumes/Data/w/external/clang+llvm-13.0.0-x86_64-apple-darwin/bin/:$PATH
// function compile() { clang -glldb -Werror -Wduplicate-enum -Wshadow-all -O0 -fsanitize=address -F /Library/Frameworks/ -c $1.cpp -MJ build/$1.o.compdb -o build/$1.o; }
// function compile() { clang -glldb -Werror -Wduplicate-enum -Wshadow-all -flto -Ofast -F /Library/Frameworks/ -c $1.cpp -MJ build/$1.o.compdb -o build/$1.o; }
// compile px_asteroids
// clang -gembed-source -gfull -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec
// compile px_asteroids; compile soft_window; clang -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec


#include <cstdlib>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "soft_window.h"

template<class T> T * recast(void * memory, s64 offset = 0) { return (T*)((u8*)memory + offset); }

#define HOTFUNCTION __attribute__((no_sanitize("address")))

struct Pixel { u8 red, grn, blu, alf; };

static inline Pixel * get_pixel(s16 px_x, s16 px_y) {
    if (px_x >= 0 && px_x < SoftWindow::width && px_y >= 0 && px_y < SoftWindow::height) {
        return recast<Pixel>(SoftWindow::buffer, px_y * SoftWindow::pitch + px_x * 4);
    } else {
        return 0;
    }
}

HOTFUNCTION
static inline Pixel * set_pixel(s16 px_x, s16 px_y, Pixel px) {
    auto pxp = get_pixel(px_x, px_y);
    if (pxp) { *pxp = px; }
    return pxp;
}

HOTFUNCTION
static inline void draw_box(s32 l, s32 t, s32 w, s32 h, Pixel color) {
    for (int box_px_y = t; box_px_y < t + h; box_px_y++) {
        for (int box_px_x = l; box_px_x < l + w; box_px_x++) {
            set_pixel(box_px_x, box_px_y, color);
        }
    }
}

static Pixel white = {0xff, 0xff, 0xff, 0xff};
static Pixel megenta = {0xff, 0x5f, 0xff, 0xff};

typedef float Vector2 __attribute__((ext_vector_type(2)));
typedef float Mat3x3 __attribute__((matrix_type(3, 3)));
typedef float Mat3x1 __attribute__((matrix_type(3, 1)));
typedef float Mat1x3 __attribute__((matrix_type(1, 3)));

Mat3x3 affine_identity() {
    Mat3x3 o;
    o[0][0] = 1; o[0][1] = 0; o[0][2] = 0;
    o[1][0] = 0; o[1][1] = 1; o[1][2] = 0;
    o[2][0] = 0; o[2][1] = 0; o[2][2] = 1;
    return o;
}


void affine_translate(Mat3x3 & m, float x, float y) {
    Mat3x3 o;
    o[0][0] = 1; o[0][1] = 0; o[0][2] = x;
    o[1][0] = 0; o[1][1] = 1; o[1][2] = y;
    o[2][0] = 0; o[2][1] = 0; o[2][2] = 1;
    m = o * m;
}
void affine_translate(Mat3x3 & m, Vector2 vec) {
    affine_translate(m, vec.x, vec.y);
}
void affine_rotate(Mat3x3 & m, float t) {
    Mat3x3 o;
    auto r0 = cos(t);
    auto r1 = sin(t);
    o[0][0] = r0; o[0][1] = -r1; o[0][2] = 0;
    o[1][0] = r1; o[1][1] = r0; o[1][2] = 0;
    o[2][0] = 0; o[2][1] = 0; o[2][2] = 1;
    m = o * m;
}


static inline void draw_line(Vector2 v0, Vector2 v1, Pixel color) {
    s16 x0 = v0.x, y0 = v0.y, x1 = v1.x, y1 = v1.y;
    auto const dx =  abs(x1-x0);
    auto const sx = x0 < x1 ? 1 : -1;
    auto const dy = -abs(y1-y0);
    auto const sy = y0 < y1 ? 1 : -1;
    auto err = dx + dy;
    set_pixel(x0, y0, color);
    while (x0 != x1 || y0 != y1) {
        auto const e2 = 2 * err;
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


struct Vector2Pointer {
    Vector2 * vecs; u32 len;
    template<int len_> Vector2Pointer(Vector2 (&vecs_)[len_]) { vecs=vecs_; len = len_; }
};

// Ideal syntax should allow this: return mat * { vec, 1 } ... or something else that is consise...
Vector2 vec2_project_point(Vector2 vec, Mat3x3 mat) {
    Mat3x1 vec_m; vec_m[0][0] = vec.x; vec_m[1][0] = vec.y; vec_m[2][0] = 1;
    auto projected = mat * vec_m;
    return {projected[0][0], projected[1][0]};
}

static inline void draw_lines(Vector2Pointer vecs, Mat3x3 at) {
    for (int i = 1; i < vecs.len; i++) {
        draw_line(
            vec2_project_point(vecs.vecs[i-1], at),
            vec2_project_point(vecs.vecs[i-0], at),
            white);
    }
}

Vector2 ship_mesh[] = { {-10, 10}, { 0, -10}, { 10, 10}, { -10, 10}, };
Vector2 asteroid_mesh[] = {
    {  0, -20},
    {  5, -15},
    { 10, -15},
    { 15, -15},
    { 20, -10},
    { 15, -10},
    { 15,  -5},
    { 10,  -5},
    { 10,   0},
    {  5,   0},
    {  5,   5},
    {  0,   5},
    {  0,  10},
    {  0, -20},
};


struct Asteroid {
    Vector2 position; Vector2 velocity;
    float rotation; float rotation_speed;
} asteroids[1024];
int asteroid_count;

float rand11() { return 2.0f * (float)random()/(float)RAND_MAX - 1.0f; }

void asteroid_spawn() {
    if (asteroid_count < 1024) {
        asteroids[asteroid_count++] = {
            {float(random()%SoftWindow::width), float(random()%SoftWindow::height)},
            {rand11() *.03f, rand11() *.03f},
            rand11() * 1.f, rand11() *.001f,
        };
    }
}

float wrap(float v, float min, float max) {
    auto d = max - min;
    if (v < min) v += d;
    if (v > max) v -= d;
    return v;
}

int main() {

    SoftWindow::init();
    int should_keep_running = 1;
    float ship_x = 50;
    float ship_y = 50;
    float ship_r = 0;
    float ship_rd = 0;
    float ship_xd = 0;
    float ship_yd = 0;


    asteroid_spawn();
    asteroid_spawn();
    asteroid_spawn();
    asteroid_spawn();
    asteroid_spawn();



    while (should_keep_running) {
        using namespace SoftWindow;
        draw_box(0, 0, width, height, megenta);
        //draw_box(box_x, box_y, 10, 10, white);
        //draw_line(200, 200, ship_x, ship_y, white);

        for (int i=0; i< asteroid_count; i++ ) {
            auto& a = asteroids[i];

            auto m = affine_identity();
            affine_rotate(m, a.rotation);
            affine_translate(m, a.position);
            draw_lines(asteroid_mesh, m);
        }


        {
            auto m = affine_identity();
            affine_rotate(m, ship_r);
            affine_translate(m, ship_x, ship_y);
            draw_lines(ship_mesh, m);
        }

        SoftWindow::update();
        set_title("%dms\n", delta_ms);

        if (SoftWindow::key_presses[KEY_QUIT]) should_keep_running = 0;

        ship_rd += (key_downs[ KEY_RIGHT ]-key_downs[ KEY_LEFT  ]) * delta_ms * 0.00001f ;
        ship_r += ship_rd * delta_ms;

        float thrust = (key_downs[ KEY_UP ]-key_downs[ KEY_DOWN  ]) * delta_ms * 0.0001f ;
        ship_xd += sinf(ship_r) * thrust;
        ship_yd -= cosf(ship_r) * thrust;

        ship_x += ship_xd * delta_ms;
        ship_y += ship_yd * delta_ms;

        if (ship_rd < 0) {
            ship_rd += delta_ms * 0.001f * -ship_rd;
        } else {
            ship_rd -= delta_ms * 0.001f *  ship_rd;
        }

        ship_x = wrap(ship_x, 0, width);
        ship_y = wrap(ship_y, 0, height);
        ship_r = wrap(ship_r, 0, 2 * M_PI);

        for (int i=0; i< asteroid_count; i++ ) {
            auto& a = asteroids[i];
            a.position += a.velocity * delta_ms;
            a.rotation += a.rotation_speed * delta_ms;
            a.position.x = wrap(a.position.x, 0, width);
            a.position.y = wrap(a.position.y, 0, height);
            a.position.r = wrap(a.position.r, 0, 2 * M_PI);
        }


    }
}
