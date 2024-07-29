#pragma once
#include "primitive.h"
#include "gfx.h"
#include "util.h"

class Canvas {

  public:

    Canvas() :
      fontSheet("data/font.png", 16, 16) {}

    void setColor(float r, float g, float b, float a=1.0) { prim.setColor(r, g, b, a); }
    void setScroll(const Point& p) {
        currentScroll = p;
        prim.setScroll(currentScroll.x, currentScroll.y);
    }
    void setScroll(int x, int y) {
        currentScroll = Point{(double)x, (double)y};
        prim.setScroll(currentScroll.x, currentScroll.y);
    }
    void addScroll(int x, int y) {
        currentScroll = Point{currentScroll.x + x, currentScroll.y + y};
        prim.setScroll(currentScroll.x, currentScroll.y);
    }
    void pushScroll() {
        scrollStack.push_back(currentScroll);
    }
    void popScroll() {
        currentScroll = scrollStack.back();
        scrollStack.pop_back();
        prim.setScroll(currentScroll.x, currentScroll.y);
    }
    void setScale(float scale) { prim.setScale(scale); }
    void drawLine(float x0, float y0, float x1, float y1) { prim.drawLine(x0, y0, x1, y1); }
    void drawRectangle(float x0, float y0, float x1, float y1) { prim.drawRectangle(x0, y0, x1, y1); }
    void drawTexture(float x0, float y0, float w, float h, float targetW, float targetH) { prim.drawTexture(x0, y0, w, h, targetW, targetH); }
    void drawConvexPolygon(const std::vector<float>& points, float textureScale, float texture_x, float texture_y) {
      prim.drawConvexPolygon(points, textureScale, texture_x, texture_y);
    }
    void drawCircle(float x, float y, float r) { prim.drawCircle(x, y, r); }
    void drawCircleOutline(float x, float y, float r) { prim.drawCircleOutline(x, y, r); }
    void print(const Point& start_pos, const std::string& text, float size=1.0) {
      int x = start_pos.x - 8 + currentScroll.x;
      int y = start_pos.y - 8 + currentScroll.y;
      const float kern = 0.8;
      for (char c: text) {
        if (c == '\n') {
          x = start_pos.x - 8;
          y += 20;
        }
        else {
          fontSheet.drawSpriteScaled(x, y, c, size);
          x += fontSheet.getFrameWidth() * kern * size;
        }
      }
    }
    void draw(Image& image, Point position, float scale = 1.0) {
      image.draw(position.x, position.y, image.getWidth() * scale, image.getHeight() * scale);
    }
    void draw(Image& image, Point position, Point size) {
      image.draw(position.x, position.y, size.x, size.y);
    }

  private:

    PrimitiveShader prim;
    SpriteSheet fontSheet;
    std::vector<Point> scrollStack;
    Point currentScroll{0.0, 0.0};

 };

