
#include "soft_window.h"

#define HOTFUNCTION __attribute__((no_sanitize("address")))

SoftWindow::Color * get_pixel(s16 px_x, s16 px_y) {
    if (px_x >= 0 && px_x < SoftWindow::width && px_y >= 0 && px_y < SoftWindow::height) {
        return SoftWindow::pixels + px_y * SoftWindow::pitch + px_x;
    } else {
        return 0;
    }
}

HOTFUNCTION
SoftWindow::Color * set_pixel(s16 px_x, s16 px_y, SoftWindow::Color px) {
    auto pxp = get_pixel(px_x, px_y);
    if (pxp) { *pxp = px; }
    return pxp;
}


HOTFUNCTION
void draw_box(s32 l, s32 t, s32 w, s32 h, SoftWindow::Color color) {
    for (int box_px_y = t; box_px_y < t + h; box_px_y++) {
        for (int box_px_x = l; box_px_x < l + w; box_px_x++) {
            set_pixel(box_px_x, box_px_y, color);
        }
    }
}


static u16 const grid_width = 8;
static u16 const grid_height = 8;

u8 grid[grid_height][grid_width];

static u16 const grid_left = 100;
static u16 const grid_top = 100;
static u16 const grid_cell_size = 20;
static u16 const grid_padding = 2;

int main () {

    bool running = 1;

    while (running) {
        draw_box(0,0,SoftWindow::width, SoftWindow::height, {0,0,0,0});

        for (int gy=0; gy<grid_width; gy++) {
            for (int gx=0; gx<grid_width; gx++) {
                if (grid[gy][gx]) {
                    draw_box(
                        grid_left + gx * (grid_cell_size + grid_padding),
                        grid_top  + gy * (grid_cell_size + grid_padding),
                        grid_cell_size, grid_cell_size,
                        {255, 255, 255, 255});
                }
            }
        }

        SoftWindow::update();
        if (SoftWindow::key_downs[SoftWindow::KEY_QUIT]) running = 0;
        if (SoftWindow::key_presses[SoftWindow::KEY_MOUSE_1]) {

            auto gx = (u16)(SoftWindow::mouse_x - grid_left) / ( grid_cell_size + grid_padding );
            auto gy = (u16)(SoftWindow::mouse_y - grid_top ) / ( grid_cell_size + grid_padding );

            if (gx < grid_width && gy < grid_height) {
                grid[gy][gx] ^= 1;
            }

        }

    }

}