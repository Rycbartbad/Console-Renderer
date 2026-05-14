
#ifndef PROJECT_TUI_H
#define PROJECT_TUI_H

#include "layer2d.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ── Base widget ──────────────────────────────────────────────────────────
class Widget {
public:
    Widget(int x, int y, int w, int h) : m_x(x), m_y(y), m_w(w), m_h(h) {}
    virtual ~Widget() = default;
    virtual void draw(Layer2D& layer) = 0;
    virtual bool handle_click(int mx, int my) { return false; }

    void set_pos(int x, int y) { m_x = x; m_y = y; }
    void set_size(int w, int h) { m_w = w; m_h = h; }
    int x() const { return m_x; }
    int y() const { return m_y; }
    int w() const { return m_w; }
    int h() const { return m_h; }

protected:
    int m_x, m_y, m_w, m_h;
};

// ── Label ────────────────────────────────────────────────────────────────
class Label : public Widget {
public:
    Label(int x, int y, const std::string& text, Vec3 fg = Vec3(200,200,200), int font_scale = 1)
        : Widget(x, y, (int)text.size() * 8 * font_scale, 8 * font_scale)
        , m_text(text), m_fg(fg), m_scale(font_scale) {}
    void draw(Layer2D& layer) override;
    void set_text(const std::string& t) { m_text = t; }
    void set_scale(int s) { m_scale = s; }
private:
    std::string m_text;
    Vec3 m_fg;
    int m_scale;
};

// ── Button ───────────────────────────────────────────────────────────────
class Button : public Widget {
public:
    Button(int x, int y, const std::string& text, std::function<void()> cb,
           Vec3 bg = Vec3(60,60,60), Vec3 fg = Vec3(220,220,220), int font_scale = 1)
        : Widget(x, y, (int)text.size() * 8 * font_scale + 4, 8 * font_scale + 4)
        , m_text(text), m_cb(cb), m_bg(bg), m_fg(fg), m_scale(font_scale) {}
    void draw(Layer2D& layer) override;
    bool handle_click(int mx, int my) override;
private:
    std::string m_text;
    std::function<void()> m_cb;
    Vec3 m_bg, m_fg;
    int m_scale;
};

// ── TUI manager ──────────────────────────────────────────────────────────
class TUI {
public:
    void add(std::unique_ptr<Widget> w) { m_widgets.push_back(std::move(w)); }

    void draw(Layer2D& layer) {
        for (auto& w : m_widgets) w->draw(layer);
    }

    bool handle_click(int mx, int my) {
        for (auto& w : m_widgets)
            if (w->handle_click(mx, my)) return true;
        return false;
    }

private:
    std::vector<std::unique_ptr<Widget>> m_widgets;
};

#endif // PROJECT_TUI_H
