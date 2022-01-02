#include "soft_window.h"
int main () {
    bool should_keep_running = 1;
    while (should_keep_running) {
        SoftWindow::update();
        if (SoftWindow::key_downs[SoftWindow::KEY_QUIT]) should_keep_running = 0;
        if (SoftWindow::key_downs[SoftWindow::KEY_MOUSE_1]) {
            *(SoftWindow::pixels + SoftWindow::mouse_x + SoftWindow::mouse_y * SoftWindow::pitch) = {
                255, 255, 255, 255
            };
        }
    }
}