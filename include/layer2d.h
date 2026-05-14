
#ifndef PROJECT_LAYER2D_H
#define PROJECT_LAYER2D_H

#include "math_utils.h"
#include <string>
#include <vector>
#include <memory>

// ── 2D console cell ──────────────────────────────────────────────────────
struct Cell {
    Vec3 bg = Vec3(0, 0, 0);       // background color
    Vec3 fg = Vec3(255, 255, 255); // foreground color
    char ch = ' ';                  // character
    bool transparent = false;      // if true, skip this cell during composite
    bool has_text = false;         // if true, output ch instead of space
};

// ── Layer2D: a z-ordered 2D drawing surface ──────────────────────────────
class Layer2D {
public:
    Layer2D(int width, int height, int z_order = 0);

    int width()  const { return m_width; }
    int height() const { return m_height; }
    int z()      const { return m_z; }
    void set_z(int z) { m_z = z; }

    void set_visible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }

    // Clear entire layer
    void clear(Vec3 bg = Vec3(0, 0, 0));

    // ── Drawing primitives (normalized coords 0..1) ──────────────────────
    // Pixel-level access (for internal / advanced use)
    void set_cell(int x, int y, char ch, Vec3 fg = Vec3(255,255,255), Vec3 bg = Vec3(0,0,0));

    // Normalized-coordinate drawing (nx, ny, nw, nh ∈ [0, 1])
    void fill_rect(float nx, float ny, float nw, float nh, Vec3 bg);
    void draw_border(float nx, float ny, float nw, float nh, Vec3 color = Vec3(200,200,200));
    void draw_text(float nx, float ny, const std::string& text, Vec3 fg = Vec3(255,255,255),
                   Vec3 bg = Vec3(0,0,0), float scale = 1.0f);
    void draw_line(float nx0, float ny0, float nx1, float ny1, Vec3 color = Vec3(200,200,200));

    const Cell& cell(int x, int y) const { return m_cells[x + y * m_width]; }
    Cell& cell(int x, int y) { return m_cells[x + y * m_width]; }

    // Composite this layer onto a Vec3 color buffer
    void composite_to(std::vector<Vec3>& target, int target_w, int target_h) const;

private:
    int m_width, m_height, m_z;
    bool m_visible = true;
    std::vector<Cell> m_cells;
};

#endif // PROJECT_LAYER2D_H
