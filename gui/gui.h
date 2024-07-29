#pragma once

#include <functional>

class GuiMenu;
class GuiElement {
    public:
        GuiElement(Canvas* canvas, int x, int y, int w = 32, int h = 32)
            : mCanvas(canvas), mX(x), mY(y), mW(w), mH(h) {}
        bool guiMouseIn(int x, int y) {
            return (x > getX() && y > getY() && x < getX() + getWidth() && y < getY() + getHeight());
        }
        int getX() { return mX; }
        int getY() { return mY; }
        virtual int getWidth() { return mW; };
        virtual int getHeight() { return mH; };
        virtual void guiEventMousePressed(bool pressed, int button, int x, int y) {
            x -= getX();
            y -= getY();
            bool handled = false;
            if (hasTitleBar) {
                if (y < 10) {
                    guiElementDragging = pressed;
                    handled = true;
                } else {
                    y -= 10;
                }
            }
            if (!handled) {
                for (auto& element : childElements) {
                    if (element->guiMouseIn(x, y)) {
                        element->guiEventMousePressed(pressed, button, x, y);
                        handled = true;
                        break;
                    }
                }
            }
            if (!handled) {
                mousePressed(pressed, button, x, y);
            }
            if (onClick) {
                onClick();
            }
        }
        virtual void guiEventMouseMove(int x, int y, int dx, int dy) {
            x -= getX();
            y -= getY();
            bool handled = false;
            if (hasTitleBar) {
                if (guiElementDragging) {
                    mX += dx;
                    mY += dy;
                    handled = true;
                } else {
                    y -= 10;
                }
            }
            if (!handled) {
                for (auto& element : childElements) {
                    if (element->guiMouseIn(mouse_x - dx, mouse_y - dy)) {
                        element->guiEventMouseMove(mouse_x, mouse_y, dx, dy);
                        handled = true;
                        break;
                    }
                }
            }
            if (!handled) {
                mouseMove(x, y, dx, dy);
            }
        }
        virtual void guiEventDraw();
        virtual void guiEventMouseWheel(int x, int y, int value) {
            x -= getX();
            y -= getY();
            bool handled = false;
            for (auto& element : childElements) {
                if (element->guiMouseIn(x, y)) {
                    element->guiEventMouseWheel(x, y, value);
                    handled = true;
                    break;
                }
            }
            mouseWheel(x, y, value);
        }
        virtual void mousePressed(bool pressed, int button, int x, int y) {}
        virtual void mouseMove(int x, int y, int dx, int dy) {}
        virtual void mouseWheel(int x, int y, int value) {}
        virtual void draw() {}
        void addElement(GuiElement* child) {
            childElements.push_back(child);
        }
        void addMenu(GuiMenu* menu) {
            mMenu = menu;
        }
    public:
        bool hasTitleBar = false;
        bool hasBackground = false;
        bool hasBorder = false;
        std::string title = "";
        std::function<void(void)> onClick;
    protected:
        int mX;
        int mY;
        int mW;
        int mH;
        Canvas* mCanvas;
        bool guiElementDragging{false};
        std::vector<GuiElement*> childElements;
        GuiMenu* mMenu = nullptr;
};


class GuiButton : public GuiElement
{
    public:
        GuiButton(Canvas* canvas, int x, int y, int w, int h, const std::string& text)
            : GuiElement{canvas, x, y, w, h}, text{ text }
        {
            hasBackground = true;
            hasBorder = true;
        }
        virtual void draw()
        {
            mCanvas->print({24, 24}, text);
        }

        std::string text;
};

class GuiInput : public GuiElement
{
    public:
        GuiInput(Canvas* canvas)
            : GuiElement{canvas, 0, 0, -1, 24}
        {
            hasBorder = true;
        }
        virtual void draw()
        {
            mCanvas->print({24, 24}, text);
        }

        std::string text;
};

class GuiLabel : public GuiElement
{
    public:
        GuiLabel(Canvas* canvas, int x, int y, const std::string& text)
            : GuiElement{canvas, x, y, 100, 24}, text(text)
        {}
        virtual void draw()
        {
            mCanvas->print({24, 24}, text);
        }

        std::string text;
};

class GuiMenu : public GuiElement {
    public:
        class GuiMenuItem {
            public:
                GuiMenuItem(const std::string& name, std::function<void(void)>) : name(name), action(action) {}
                std::string name;
                std::function<void(void)> action;
        };
        class GuiMenuItemList {
            public:
                void addItem(const std::string& name, std::function<void(void)> action) {
                    items.emplace_back(name, action);
                }
                std::vector<GuiMenuItem> items;
        };
        GuiMenu(Canvas* canvas)
            : GuiElement{canvas, 0, 0, -1, 24} {}
        GuiMenuItemList* addMenu(const std::string& name) {
            return &menus[name];
        }
        virtual void draw()
        {
            int x = 24;
            for (const auto& menu : menus) {
                mCanvas->print({x, 24}, menu.first);
                x += 100;
            }
        }
    private:
            std::map<std::string, GuiMenuItemList> menus;
};

void GuiElement::guiEventDraw() {
    mCanvas->pushScroll();
    mCanvas->addScroll(mX, mY);
    if (hasBorder) {
        mCanvas->setColor(1.0f, 1.0f, 1.0f);
        mCanvas->drawRectangle(0, 0, getWidth()-2, getHeight()-2);
        mCanvas->setColor(0.0f, 0.0f, 0.0f);
        mCanvas->drawRectangle(2, 2, getWidth(), getHeight());
    }
    if (hasBackground) {
        mCanvas->setColor(0.55f, 0.55f, 0.55f);
        mCanvas->drawRectangle(1, 1, getWidth()-1, getHeight()-1);
    }
    if (hasTitleBar) {
        mCanvas->setColor(0.0f, 0.25f, 0.55f);
        mCanvas->drawRectangle(1, 1, getWidth()-1, 16);
        mCanvas->print({24, 14}, title, 0.5);
        mCanvas->addScroll(0, 16);
    }
    if (mMenu != nullptr) {
        mCanvas->setColor(0.25f, 0.25f, 0.25f);
        mCanvas->drawRectangle(0, 0, getWidth(), 24);
//                mCanvas->print({24, 14}, title, 0.5);
        mMenu->draw();
        mCanvas->addScroll(0, 24);
    }
    for (auto element : childElements) {
        element->guiEventDraw();
    }
    draw();
    mCanvas->popScroll();
}
