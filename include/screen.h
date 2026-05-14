
#ifndef PROJECT_SCREEN_H
#define PROJECT_SCREEN_H
#include "math_utils.h"
#include <string>
#include <vector>
#include <ctime>
#include <chrono>

struct Tile {
    int x_start, x_end;
    int y_start, y_end;
};

inline std::vector<Tile> partition_tiles(const int width, const int height, int num_tiles) {
    if (num_tiles <= 1) return { { 0, width, 0, height } };

    // Adaptive grid: split into cols×rows that best match screen aspect ratio
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(num_tiles) * aspect)));
    int rows = (num_tiles + cols - 1) / cols;

    const int tile_w = (width + cols - 1) / cols;
    const int tile_h = (height + rows - 1) / rows;

    std::vector<Tile> tiles;
    tiles.reserve(static_cast<size_t>(cols) * static_cast<size_t>(rows));
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            const int x0 = c * tile_w;
            const int y0 = r * tile_h;
            const int x1 = std::min(x0 + tile_w, width);
            const int y1 = std::min(y0 + tile_h, height);
            if (x1 > x0 && y1 > y0)
                tiles.push_back({ x0, x1, y0, y1 });
        }
    }
    return tiles;
}

class Screen {
public:
    Screen();
    static void init();
    void update();
    virtual void set_pixel(const int& x, const int& y, const Vec3& color, const float& z);
    // Early-z: returns true if pixel passes depth test (no need to shade if occluded)
    [[nodiscard]] virtual bool depth_test(const int x, const int y, const float z) const;
    void clear();
    void draw();
    static void calculate_fps();
    static int get_fps() { return fps; }
    void show() const;
    void apply_ssaa();
    void apply_fxaa();
    void apply_taa();
    [[nodiscard]] int get_taa_frame_count() const { return taa_frame_count; }
    static Vec2 halton_sequence(int index);

    std::string output_buf;
    std::vector<Vec3> buffer;
    std::vector<float> z_buffer;
    std::vector<Vec3> prev_buffer;  // for diff-based output
    int width, height;
    bool bias, SSAA, show_fps;

private:
    static int counter_t, fps;
    static std::chrono::high_resolution_clock::time_point hp_start;

    std::vector<Vec3> taa_history;
    std::vector<Vec3> aa_scratch;  // reusable buffer for AA post-processing
    float taa_jitter_x = 0, taa_jitter_y = 0;
    int taa_frame_count = 0;
};
// TileScreen: presents as full-size Screen (correct projection), but only allocates
// a tile-sized local buffer. set_pixel clips to tile bounds and maps to local coords.
class TileScreen : public Screen {
public:
    int tile_x, tile_y, tile_w, tile_h;

    TileScreen(const int g_w, const int g_h, const int tx, const int ty, const int tw, const int th)
        : tile_x(tx), tile_y(ty), tile_w(tw), tile_h(th) {
        width = g_w;
        height = g_h;
        buffer.assign(tw * th, Vec3());
        z_buffer.assign(tw * th, 0.0f);
    }

    void set_pixel(const int& x, const int& y, const Vec3& color, const float& z) override {
        if (x < tile_x || x >= tile_x + tile_w || y < tile_y || y >= tile_y + tile_h)
            return;
        const int lx = x - tile_x;
        const int ly = y - tile_y;
        const size_t index = static_cast<size_t>(lx) + static_cast<size_t>(ly) * static_cast<size_t>(tile_w);
        if (z_buffer[index] != 0 && z >= z_buffer[index]) return;
        z_buffer[index] = z;
        buffer[index] = color;
    }
    [[nodiscard]] bool depth_test(const int x, const int y, const float z) const override {
        if (x < tile_x || x >= tile_x + tile_w || y < tile_y || y >= tile_y + tile_h)
            return false;
        const size_t index = static_cast<size_t>(x - tile_x) + static_cast<size_t>(y - tile_y) * static_cast<size_t>(tile_w);
        return z_buffer[index] == 0 || z < z_buffer[index];
    }
    // Reuse buffer when tile dimensions match (avoids page faults from reallocation)
    bool resize_if_needed(const int g_w, const int g_h,
                          const int tx, const int ty, const int tw, const int th) {
        if (static_cast<size_t>(tw * th) == buffer.size() && tile_w == tw && tile_h == th) {
            tile_x = tx; tile_y = ty;
            width = g_w; height = g_h;
            std::fill(buffer.begin(), buffer.end(), Vec3());
            std::fill(z_buffer.begin(), z_buffer.end(), 0.0f);
            return true;  // reused, no page fault
        }
        this->~TileScreen();
        new(this) TileScreen(g_w, g_h, tx, ty, tw, th);
        return false;
    }
};

#endif //PROJECT_SCREEN_H