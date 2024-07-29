#include <fstream>
#include <sstream>
#include <map>
#include <GLES3/gl3.h>
#include "sys/main.h"
#include "gfx/canvas.h"
#include "gfx/gfx.h"
#include "gfx/lodepng.h"
#include "glm/gtc/matrix_transform.hpp"
#include "gui/gui.h"

bool modCtrl = false;
bool modAlt = false;
bool modShift = false;

Canvas* canvas;

typedef uint8_t Pixel;

struct Color {
    float r;
    float g;
    float b;
    static Color fromHex(const std::string& hexString) {
        float r = stoi(hexString.substr(1, 2), nullptr, 16) / 255.0;
        float g = stoi(hexString.substr(3, 2), nullptr, 16) / 255.0;
        float b = stoi(hexString.substr(5, 2), nullptr, 16) / 255.0;
        return Color{r, g, b};
    }
    bool operator==(const Color& other) const { return this->r == other.r && this->g == other.g && this->b == other.b; }
};

class Palette {
    public:
        Palette() {
        }
        size_t size() { return lut.size(); }
        void setSize(int newSize) { lut.resize(newSize); }
        Color getColor(Pixel index) { return lut[index]; }
        void setColor(Pixel index, Color color) { lut[index] = color; }
        Pixel addColor(Color color) {
            auto codeIt = std::find(lut.begin(), lut.end(), color);
            if (codeIt != lut.end()) {
                return codeIt - lut.begin();
            }
            lut.push_back(color);
            return lut.size() -1;
        }
    private:
    public:
        std::vector<Color> lut;
};

class Bitmap {
    public:
        Bitmap() {
            width = 1;
            height = 1;
            data.resize(width * height);
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    pixelAt(x, y) = 0;
                }
            }
        }
        Pixel& pixelAt(int x, int y) {
            return data[x + y * width];
        }
        int getWidth() { return width; }
        int getHeight() { return height; }
        unsigned int loadpng(const std::string& filename);
        unsigned int loadXpm2(const std::string& filename);

        Palette palette;
    private:
        std::vector<Pixel> data;
        unsigned int width;
        unsigned int height;
};


unsigned int Bitmap::loadpng(const std::string& filename) {
    std::vector<unsigned char> image;
    unsigned error = lodepng::decode(image, width, height, filename);
    if(error != 0)
    {
        std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
        return 1;
    }
    data.resize(width * height);
    for(size_t i = 0; i < height * width; i++) {
        Color color{
            (image[i*4 + 0])/255.0f,
            (image[i*4 + 1])/255.0f,
            (image[i*4 + 2])/255.0f,
        };
        auto index = palette.addColor(color);
        data[i] = index;
    }
    return 0;
}

unsigned int Bitmap::loadXpm2(const std::string& filename)
{
    std::ifstream file;
    file.open(filename.c_str());
    std::string magic;
    std::getline(file, magic);
    if (magic != "! XPM2") {
        std::cout << "not xpm2" << std::endl;
        return 1;
    }
    int colors;
    int cpp; // characters per pixel
    file >> width;
    file >> height;
    file >> colors;
    file >> cpp;
    std::vector<std::string> codes;
    palette.setSize(colors);
    for (int i = 0; i < colors; ++i) {
        std::string code;
        std::string type;
        std::string hexColor;
        file >> code >> type >> hexColor;
        codes.push_back(code);
        auto color = Color::fromHex(hexColor);
        palette.setColor(i, color);
    }
    std::string line;
    data.resize(width * height);
    int dataIndex = 0;
    while (std::getline(file, line)) {
        for (int i = 0; i < line.size() / cpp; ++i) {
            std::string code = line.substr(i*cpp, cpp);
            auto codeIt = std::find(codes.begin(), codes.end(), code);
            int paletteIndex = 0;
            if (codeIt != codes.end()) {
                paletteIndex = codeIt - codes.begin();
            }
            data[dataIndex] = paletteIndex;
            dataIndex += 1;
        }
    }
    file.close();
    return 0;
}

class ImageViewMini : public GuiElement {
    public:
        ImageViewMini(Canvas* canvas, int x, int y, Bitmap* image)
            : GuiElement(canvas, x, y)
            , mImage(image) {}
        int getWidth() override { return mImage->getWidth(); }
        int getHeight() override { return mImage->getHeight(); }
        void draw() override {
            //mCanvas->draw(mImage);
        }
    private:
        Bitmap* mImage;
};

class ImageView : public GuiElement {
    public:
        ImageView(Canvas* canvas, int x, int y, int w, int h, Bitmap* image, int& selectedIndex, int &altIndex)
            : GuiElement(canvas, x, y, w, h)
            , mImage(image)
            , mSelectedIndex(selectedIndex)
            , mAltIndex(altIndex)
        {
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            updateTexture();
        }
        int getWidth() override { return mPixelSize * mImage->getWidth(); }
        int getHeight() override { return mPixelSize * mImage->getHeight(); }
        void mousePressed(bool pressed, int button, int x, int y) override {
            if (modCtrl) {
                // control enables color picker
                int px = x / mPixelSize;
                int py = y / mPixelSize;
                auto newIndex = mImage->pixelAt(px, py);
                if (button == 1) {
                    mSelectedIndex = newIndex;
                } else {
                    mAltIndex = newIndex;
                }
            } else {
                mPainting = pressed;
                mPaintIndex = (button == 1 ? mSelectedIndex : mAltIndex);
                mouseMove(x, y, 0, 0);
            }
        }
        void mouseMove(int x, int y, int dx, int dy) override {
            if (mPainting) {
                x /= mPixelSize;
                y /= mPixelSize;
                mImage->pixelAt(x, y) = mPaintIndex;
                textureOutOfDate = true;
            }
        }
        void updateTexture() {
            int w = mImage->getWidth();
            int h = mImage->getHeight();
            uint8_t* data = new uint8_t[w * h * 4];
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    auto pixel = mImage->pixelAt(x, y);
                    auto color = mImage->palette.getColor(pixel);
                    data[(y * w + x)*4 + 0] = color.r * 255;
                    data[(y * w + x)*4 + 1] = color.g * 255;
                    data[(y * w + x)*4 + 2] = color.b * 255;
                    data[(y * w + x)*4 + 3] = 255;
                }
            }
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
            delete[] data;
        }

        void draw() override {
            glScissor(20, screen_h - mH - mY, mW, mH);
            glEnable(GL_SCISSOR_TEST);
            if (textureOutOfDate) {
                updateTexture();
                textureOutOfDate = false;
            }
            int w = mImage->getWidth();
            int h = mImage->getHeight();
            glBindTexture(GL_TEXTURE_2D, textureId);
            canvas->drawTexture(0, 0, w*mPixelSize, h*mPixelSize, w, h);
            canvas->drawTexture(w*mPixelSize, 0, w*4, h*4, w, h);
            canvas->drawTexture(w*mPixelSize + w*4, 0, w, h, w, h);
            canvas->setColor(0,0,0);
            for (int y = 0; y <= h; ++y) {
                canvas->drawLine(0, y * mPixelSize, w * mPixelSize, y * mPixelSize);
            }
            for (int x = 0; x <= w; ++x) {
                canvas->drawLine(x * mPixelSize, 0, x * mPixelSize, h * mPixelSize);
            }
            glDisable(GL_SCISSOR_TEST);
        }
        void mouseWheel(int x, int y, int value) {
            if (value > 0) {
                zoomIn();
            } else if (value < 0) {
                zoomOut();
            }
        }
        void zoomIn() { mPixelSize += 1; }
        void zoomOut() { mPixelSize -= 1; }
    private:
        Bitmap* mImage;
        int& mSelectedIndex;
        int& mAltIndex;
        int mPixelSize = 8;
        int mPaintIndex = 0;
        bool mPainting = false;
        GLuint textureId;
        bool textureOutOfDate = true;
};

class PaletteView : public GuiElement {
    public:
        PaletteView(Canvas* canvas, int x, int y, Palette* palette, int& selectedIndex, int &altIndex)
            : GuiElement(canvas, x, y)
            , mPalette(palette)
            , mSelectedIndex(selectedIndex)
            , mAltIndex(altIndex)
        {
            hasTitleBar = true;
        }
        int getWidth() override { return mPaletteEntrySize; }
        int getHeight() override { return mPaletteEntrySize * mPalette->size(); }
        void mousePressed(bool pressed, int button, int x, int y) override {
            int newIndex = y / mPaletteEntrySize;
            if (modCtrl) {
                std::cout << "Now starting adjusting" << std::endl;
                mAdjusting = pressed;
                mAdjustingIndex = newIndex;
                mAdjustingOrigin = Point{(double)x, (double)y};
            }
            else {
                if (button == 1) {
                    mSelectedIndex = newIndex;
                } else {
                    mAltIndex = newIndex;
                }
            }
        }
        void mouseMove(int x, int y, int dx, int dy) override {
            if (mAdjusting) {
                std::cout << "Now adjusting" << std::endl;
                Color newColor = mPalette->getColor(mAdjustingIndex);
                newColor.r = (mAdjustingOrigin.x - x)/50.0;
                newColor.g = (mAdjustingOrigin.y - y)/50.0;
                mPalette->setColor(mAdjustingIndex, newColor);
            }
        }
        void draw() override {
            for (int i = 0; i < mPalette->size(); ++i) {
                auto color = mPalette->getColor(i);
                const int paletteEntrySize = 50;
                int border = 1;
                if (i == mSelectedIndex) {
                    mCanvas->setColor(1, 0, 0);
                    border = 5;
                } else if (i == mAltIndex) {
                    mCanvas->setColor(1, 1, 0);
                    border = 5;
                } else {
                    mCanvas->setColor(0, 0, 0);
                }
                mCanvas->drawRectangle(
                        0, 0,
                        paletteEntrySize, paletteEntrySize);
                mCanvas->setColor(color.r, color.g, color.b);
                mCanvas->drawRectangle(
                        border, border,
                        paletteEntrySize-border, paletteEntrySize-border);
                mCanvas->addScroll(0, paletteEntrySize);
                if (mAdjusting) {
                }
            }
        }
    private:
        Palette* mPalette;
        int& mSelectedIndex;
        int& mAltIndex;
        int mPaletteEntrySize = 50;
        bool mAdjusting = false;
        int mAdjustingIndex = 0;
        Point mAdjustingOrigin{0, 0};
};

GuiElement* gui;

Bitmap* image = nullptr;
ImageView* imageView = nullptr;
PaletteView* paletteView = nullptr;
int selectedIndex = 1;
int altIndex = 0;

void key_press(bool pressed, unsigned char key, unsigned short code) {
    std::cout << "key_pressed " << (int) key << ", " << (int) code << std::endl;
    if (code == 224 || code == 228) {
        modCtrl = pressed;
    }
    if (code == 225 || code == 229) {
        modShift = pressed;
    }
    if (code == 226 || code == 230) {
        modAlt = pressed;
    }
}

void mouse_button(bool pressed, int button, int x, int y ) {
    gui->guiEventMousePressed(pressed, button, x, y);
}

void mouse_move(int dx, int dy) {
    gui->guiEventMouseMove(mouse_x, mouse_y, dx, dy);
}

void mouse_wheel(int value) {
    gui->guiEventMouseWheel(mouse_x, mouse_y, value);
}

void joy_button(bool pressed, int button) {
}

bool gameLoop() {
    glClear(GL_COLOR_BUFFER_BIT);
    gui->guiEventDraw();
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
  canvas = new Canvas;
  image = new Bitmap;
  //image->loadXpm2("data/test.xpm2");
  image->loadpng("data/brick.png");
  //image->loadpng("data/smallFont.png");
  imageView = new ImageView(canvas, 10, 0, 450, 450, image, selectedIndex, altIndex);
  paletteView = new PaletteView(canvas, screen_w - 10 -50, 10, &image->palette, selectedIndex, altIndex);
  gui = new GuiElement(canvas, 0, 0, screen_w, screen_h);
  gui->addElement(imageView);
  gui->addElement(paletteView);
  auto mainMenu = new GuiMenu(canvas);
  auto fileMenu = mainMenu->addMenu("File");
  fileMenu->addItem("Quit", [](){});
  gui->addMenu(mainMenu);

  auto toolbar = new GuiElement(canvas, 500, 100, 8 + 32 + 8 + 32 + 8, 16 + 8 + 32 + 8);
  toolbar->hasBorder = true;
  toolbar->hasTitleBar = true;
  toolbar->hasBackground = true;
  toolbar->title = "Zoom level";
  auto buttonA = new GuiButton(canvas, 8, 8, 32, 32, "+");
  buttonA->onClick = []{ imageView->zoomIn(); };
  toolbar->addElement(buttonA);
  auto buttonB = new GuiButton(canvas, 8+32+8, 8, 32, 32, "-");
  buttonB->onClick = []{ imageView->zoomOut(); };
  toolbar->addElement(buttonB);
  gui->addElement(toolbar);
}

void gameCleanup() {
    delete gui;
    delete paletteView;
    delete imageView;
    delete image;
    delete canvas;
}

