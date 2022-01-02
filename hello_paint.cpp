#include "soft_window.h"
int main () {
    for (;;) {
        SoftWindow::update();
        if (SoftWindow::key_downs[SoftWindow::KEY_MOUSE_0]) {
            *(SoftWindow::pixels + SoftWindow::mouse_dx + SoftWindow::mouse_dy * SoftWindow::pitch) = {
                255, 255, 255, 255
            };
        }
    }
}