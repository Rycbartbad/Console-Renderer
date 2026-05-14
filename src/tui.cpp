#include "tui.h"

void Label::draw(Layer2D& layer) {
    layer.draw_text(m_x, m_y, m_text, m_fg, Vec3(0,0,0), m_scale);
}

void Button::draw(Layer2D& layer) {
    layer.fill_rect(m_x, m_y, m_w, m_h, m_bg);
    layer.draw_border(m_x, m_y, m_w, m_h, m_fg);
    layer.draw_text(m_x + 2, m_y + 2, m_text, m_fg, m_bg, m_scale);
}

bool Button::handle_click(int mx, int my) {
    if (mx >= m_x && mx < m_x + m_w && my >= m_y && my < m_y + m_h) {
        if (m_cb) m_cb();
        return true;
    }
    return false;
}
