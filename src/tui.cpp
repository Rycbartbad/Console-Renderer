#include "tui.h"

void Label::draw(Layer2D& layer) {
    layer.draw_text(m_nx, m_ny, m_text, m_fg, Vec3(0,0,0), m_scale);
}

void Button::draw(Layer2D& layer) {
    layer.fill_rect(m_nx, m_ny, m_nw, m_nh, m_bg);
    layer.draw_border(m_nx, m_ny, m_nw, m_nh, m_fg);
    layer.draw_text(m_nx + 0.002f, m_ny + 0.005f, m_text, m_fg, m_bg, m_scale);
}

bool Button::handle_click(float nmx, float nmy) {
    if (nmx >= m_nx && nmx < m_nx + m_nw && nmy >= m_ny && nmy < m_ny + m_nh) {
        if (m_cb) m_cb();
        return true;
    }
    return false;
}
