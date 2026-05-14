
#ifndef PROJECT_TUI_H
#define PROJECT_TUI_H

#include "layer2d.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ── Base widget (normalized coords 0..1) ────────────────────────────────
class Widget {
public:
    Widget(float nx, float ny, float nw, float nh) : m_nx(nx), m_ny(ny), m_nw(nw), m_nh(nh) {}
    virtual ~Widget() = default;
    virtual void draw(Layer2D& layer) = 0;
    virtual bool handle_click(float nmx, float nmy) { return false; }

    void set_pos(float nx, float ny) { m_nx = nx; m_ny = ny; }
    void set_size(float nw, float nh) { m_nw = nw; m_nh = nh; }

protected:
    float m_nx, m_ny, m_nw, m_nh;
};

// ── Label ────────────────────────────────────────────────────────────────
class Label : public Widget {
public:
    Label(float nx, float ny, const std::string& text, Vec3 fg = Vec3(200,200,200), float font_scale = 0.03f)
        : Widget(nx, ny, text.size() * 8 * font_scale / 960.0f, 8 * font_scale / 330.0f)
        , m_text(text), m_fg(fg), m_scale(font_scale) {}
    void draw(Layer2D& layer) override;
    void set_text(const std::string& t) { m_text = t; }
private:
    std::string m_text;
    Vec3 m_fg;
    float m_scale;
};

// ── Button ───────────────────────────────────────────────────────────────
class Button : public Widget {
public:
    Button(float nx, float ny, const std::string& text, std::function<void()> cb,
           Vec3 bg = Vec3(60,60,60), Vec3 fg = Vec3(220,220,220), float font_scale = 0.03f)
        : Widget(nx, ny, (text.size() * 8 * font_scale + 4) / 960.0f, (8 * font_scale + 4) / 330.0f)
        , m_text(text), m_cb(cb), m_bg(bg), m_fg(fg), m_scale(font_scale) {}
    void draw(Layer2D& layer) override;
    bool handle_click(float nmx, float nmy) override;
private:
    std::string m_text;
    std::function<void()> m_cb;
    Vec3 m_bg, m_fg;
    float m_scale;
};

// ── TUI manager ──────────────────────────────────────────────────────────
class TUI {
public:
    void add(std::unique_ptr<Widget> w) { m_widgets.push_back(std::move(w)); }

    void draw(Layer2D& layer) {
        for (auto& w : m_widgets) w->draw(layer);
    }

    bool handle_click(float nmx, float nmy) {
        for (auto& w : m_widgets)
            if (w->handle_click(nmx, nmy)) return true;
        return false;
    }

private:
    std::vector<std::unique_ptr<Widget>> m_widgets;
};

#endif // PROJECT_TUI_H
