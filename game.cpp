#include <GLES3/gl3.h>
#include "sys/main.h"
#include "gfx/canvas.h"
#include "gfx/gfx.h"



void key_press(bool pressed, unsigned char key, unsigned short code) {
}

void mouse_button(bool pressed, int button, int x, int y ) {
}

void mouse_wheel(int value) {
}

void joy_button(bool pressed, int button) {
}

bool gameLoop() {
    return true;
}

void gameInit() {
  const int w = 1600;
  const int h  = (w/16.0)*9.0;
  createWindow(w, h, PROJECT_NAME);
  glViewport(0, 0, screen_w, screen_h);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(101/255.0 * 0.5,
               184/255.0 * 0.5,
               227/225.0 * 0.5, 1.0);
}

void gameCleanup() {
}

