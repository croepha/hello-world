
// export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
// PATH=/System/Volumes/Data/w/external/clang+llvm-13.0.0-x86_64-apple-darwin/bin/:$PATH
// function compile() { clang -glldb -Werror -Wduplicate-enum -Wshadow-all -O0 -fsanitize=address -F /Library/Frameworks/ -c $1.cpp -MJ build/$1.o.compdb -o build/$1.o; }
// function compile() { clang -glldb -Werror -Wduplicate-enum -Wshadow-all -flto -Ofast -F /Library/Frameworks/ -c $1.cpp -MJ build/$1.o.compdb -o build/$1.o; }
// compile px_asteroids
// clang -gembed-source -gfull -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec
// compile px_asteroids; compile soft_window; clang -glldb -fuse-ld=lld -fsanitize=address -F /Library/Frameworks/ -framework SDL2 build/soft_window.o build/px_asteroids.o -o build/px_asteroids.exec


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

static inline void draw_line(s16 x0, s16 y0, s16 x1, s16 y1, Pixel color) {
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

static Pixel white = {0xff, 0xff, 0xff, 0xff};
static Pixel megenta = {0xff, 0x5f, 0xff, 0xff};

struct AffineTransform {
    float a, b, c,
          d, e, f,
          g, h, i;

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


struct Vector { float x, y; };
Vector vec_project_point(Vector v, AffineTransform tf) {
    return {
        v.x * tf.a + v.y * tf.b + 1.0f * tf.c,
        v.x * tf.d + v.y * tf.e + 1.0f * tf.f,
    };
}

static inline void draw_line(Vector v0, Vector v1, Pixel color) {
    draw_line(v0.x, v0.y, v1.x, v1.y, color);
}

typedef float Vector2 __attribute__((ext_vector_type(2)));
typedef float Mat3x3 __attribute__((matrix_type(3, 3)));
typedef float Mat3x1 __attribute__((matrix_type(3, 1)));
typedef float Mat1x3 __attribute__((matrix_type(1, 3)));

static inline void draw_line(Vector2 v0, Vector2 v1, Pixel color) {
    draw_line(v0.x, v0.y, v1.x, v1.y, color);
}


struct VectorPointer {
    Vector * vecs; u32 len;
    template<int len_> VectorPointer(Vector (&vecs_)[len_]) { vecs=vecs_; len = len_; }
};
static inline void draw_lines(VectorPointer vecs, AffineTransform at) {
    for (int i = 1; i < vecs.len; i++) {
        draw_line(
            vec_project_point(vecs.vecs[i-1], at),
            vec_project_point(vecs.vecs[i-0], at),
            white);
    }
}

struct Vector2Pointer {
    Vector2 * vecs; u32 len;
    template<int len_> Vector2Pointer(Vector2 (&vecs_)[len_]) { vecs=vecs_; len = len_; }
};

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




// typedef float v2 __attribute__((ext_vector_type(2)));
// typedef float m4x4_t __attribute__((matrix_type(1, 1)));

// struct mmm {

//     int eee;
//     int bbb;

//     int & operator = (int v) {
//         eee = v;
//         return bbb;
//     }
//     operator int&()  {
//         return bbb;
//     };

// };

// template<class T> struct shortcut {
//     T & ref;
//     shortcut(T & ref_) : ref(ref_) {}
//     T & operator = (T v) { return ref = v; }
//     operator T & ()  { return ref; };
// };

// struct m {
//     int v[4];
//     shortcut<int> a {v[0]};
// };


int main() {

    // m a = {};
    // a.a = 1;
    // int v = a.a;
    // printf("%d %d %d\n", (int)a.a, a.v[0], v);


    // v2 v;
    // v.xy = 0;
    // v.yx = 0;

    // m4x4_t m2;

    // m2[0][0]= 0;

    // mmm a = {};
    // int & v = a;
    // printf("%d\n", a = 3);
    // printf("%d %d\n", a.eee, a.bbb);
    // v = 2;
    // printf("%d %d\n", a.eee, a.bbb);
    // printf("%d\n", a = 4);
    // printf("%d %d\n", a.eee, a.bbb);
    // a = {6,7};
    // printf("%d %d\n", a.eee, a.bbb);
    // printf("%d\n", a = 4);




    // exit(-1);



    SoftWindow::init();
    int should_keep_running = 1;
    float ship_x = 50;
    float ship_y = 50;
    float ship_r = 0;
    float ship_rd = 0;
    float ship_xd = 0;
    float ship_yd = 0;



    while (should_keep_running) {
        using namespace SoftWindow;
        draw_box(0, 0, width, height, megenta);
        //draw_box(box_x, box_y, 10, 10, white);
        //draw_line(200, 200, ship_x, ship_y, white);


        Vector2 ship_mesh[] = { {-10, 10}, { 0, -10}, { 10, 10}, { -10, 10}, };
        {
            auto box_m = AffineTransform::identity();
            box_m.rotate(ship_r);
            box_m.translate(ship_x, ship_y);

            Mat3x3 m2;
            m2[0][0] = box_m.a;
            m2[0][1] = box_m.b;
            m2[0][2] = box_m.c;
            m2[1][0] = box_m.d;
            m2[1][1] = box_m.e;
            m2[1][2] = box_m.f;
            m2[2][0] = box_m.g;
            m2[2][1] = box_m.h;
            m2[2][2] = box_m.i;

            draw_lines(ship_mesh, m2);
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

        if (ship_x > width ) ship_x -= width;
        if (ship_x < 0     ) ship_x += width;
        if (ship_y > height) ship_y -= height;
        if (ship_y < 0     ) ship_y += height;

    }
}
