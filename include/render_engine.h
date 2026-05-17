// ─────────────────────────────────────────────────────────────────────────────
//  render_engine.h  —  Single-header console 3D render engine
//
//  Usage:
//    #define RENDER_ENGINE_IMPLEMENTATION
//    #include "render_engine.h"
//
//  Include WITHOUT the define to use declarations only (for multi-TU projects).
//
//  Dependencies: C++20, Windows (conio.h / windows.h) or Linux (termios, ioctl)
//  No external GPU/API required — renders to the console via ANSI escape codes.
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

// ============================================================
//  System includes (consolidated from all original headers)
// ============================================================
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

// ============================================================
//  Constants & Math utilities
// ============================================================
constexpr float pi = 3.14159f;

template<typename T>
[[nodiscard]] constexpr T
clamp(const T& v, const T& lo, const T& hi) {
    return std::max(lo, std::min(v, hi));
}

template<typename T>
[[nodiscard]] constexpr T
lerp(const T& a, const T& b, float t) {
    return a + (b - a) * t;
}

// Fast inverse sqrt (one Newton iteration, ~4x faster than 1/sqrtf)
[[nodiscard]] static inline float
fast_rsqrt(float x) {
    float h = 0.5f * x;
    uint32_t i = std::bit_cast<uint32_t>(x);
    i = 0x5f3759df - (i >> 1);
    x = std::bit_cast<float>(i);
    x *= (1.5f - h * x * x);
    return x;
}

// ── Vec2 ──────────────────────────────────────────────────────
class Vec2 {
  public:
    Vec2()
      : x(0)
      , y(0) {}
    Vec2(int x, int y)
      : x(x)
      , y(y) {}
    Vec2 operator+(const Vec2& b) const { return { x + b.x, y + b.y }; }
    Vec2 operator-(const Vec2& b) const { return { x - b.x, y - b.y }; }
    Vec2 operator*(int b) const { return { x * b, y * b }; }
    int cross(const Vec2& b) const { return x * b.y - y * b.x; }
    int x, y;
};

// ── Vec3 (arithmetic type, also used for RGB computation) ─────
class Vec3 {
  public:
    Vec3()
      : x(0)
      , y(0)
      , z(0) {}
    Vec3(int x, int y, int z)
      : x(x)
      , y(y)
      , z(z) {}
    Vec3 operator+(const Vec3& b) const { return { x + b.x, y + b.y, z + b.z }; }
    Vec3 operator-(const Vec3& b) const { return { x - b.x, y - b.y, z - b.z }; }
    Vec3 operator*(int b) const { return { x * b, y * b, z * b }; }
    Vec3 operator*(float b) const {
        return { static_cast<int>(x * b), static_cast<int>(y * b), static_cast<int>(z * b) };
    }
    Vec3 operator/(int b) const { return { x / b, y / b, z / b }; }
    [[nodiscard]] Vec3 hadamard(const Vec3& b) const { return { x * b.x, y * b.y, z * b.z }; }
    int operator*(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }
    bool operator==(const Vec3& b) const { return x == b.x && y == b.y && z == b.z; }
    int x, y, z;
};

// ── Color (compact 4-byte storage, format: 0x00RRGGBB) ───────
using Color = uint32_t;
[[nodiscard]] static inline Color
pack_color(const Vec3& v) {
    return ((uint32_t)clamp(v.x, 0, 255) << 16) | ((uint32_t)clamp(v.y, 0, 255) << 8) | (uint32_t)clamp(v.z, 0, 255);
}
[[nodiscard]] static inline Vec3
unpack_color(Color c) {
    return Vec3((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}

// ── Vec4 ───────────────────────────────────────────────────────
class Vec4 {
  public:
    Vec4()
      : x(0)
      , y(0)
      , z(0)
      , w(0) {}
    Vec4(float x, float y, float z, float w)
      : x(x)
      , y(y)
      , z(z)
      , w(w) {}
    Vec4 operator+(const Vec4& b) const { return { x + b.x, y + b.y, z + b.z, 1 }; }
    Vec4 operator-(const Vec4& b) const { return { x - b.x, y - b.y, z - b.z, 1 }; }
    Vec4 operator*(float b) const { return { x * b, y * b, z * b, w * b }; }
    float operator*(const Vec4& b) const { return x * b.x + y * b.y + z * b.z + w * b.w; }
    Vec3 to_vec3() const {
        return { static_cast<short>(round(x)), static_cast<short>(round(y)), static_cast<short>(round(z)) };
    }
    void normalize() {
        float sum = sqrt(x * x + y * y + z * z);
        if (sum > 0.00001f) {
            x /= sum;
            y /= sum;
            z /= sum;
        }
        w = 0;
    }
    [[nodiscard]] Vec4 cross3(const Vec4& b) const {
        return { y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x, 1 };
    }
    Vec4 operator/(float b) const { return { x / b, y / b, z / b, w / b }; }
    float x, y, z, w;
};

// ── Mat4 ───────────────────────────────────────────────────────
class Mat4 {
  public:
    Mat4()
      : mat{ Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0), Vec4(0, 0, 0, 1) } {}
    Mat4(const Vec4& x, const Vec4& y, const Vec4& z, const Vec4& w)
      : mat{ x, y, z, w } {}
    Vec4 operator*(const Vec4& b) const {
        return { mat[0].x * b.x + mat[0].y * b.y + mat[0].z * b.z + mat[0].w * b.w,
                 mat[1].x * b.x + mat[1].y * b.y + mat[1].z * b.z + mat[1].w * b.w,
                 mat[2].x * b.x + mat[2].y * b.y + mat[2].z * b.z + mat[2].w * b.w,
                 mat[3].x * b.x + mat[3].y * b.y + mat[3].z * b.z + mat[3].w * b.w };
    }
    Mat4 operator*(const Mat4& b) const {
        Mat4 t;
        for (int i = 0; i < 4; i++) {
            t.mat[i].x = mat[i].x * b.mat[0].x + mat[i].y * b.mat[1].x + mat[i].z * b.mat[2].x + mat[i].w * b.mat[3].x;
            t.mat[i].y = mat[i].x * b.mat[0].y + mat[i].y * b.mat[1].y + mat[i].z * b.mat[2].y + mat[i].w * b.mat[3].y;
            t.mat[i].z = mat[i].x * b.mat[0].z + mat[i].y * b.mat[1].z + mat[i].z * b.mat[2].z + mat[i].w * b.mat[3].z;
            t.mat[i].w = mat[i].x * b.mat[0].w + mat[i].y * b.mat[1].w + mat[i].z * b.mat[2].w + mat[i].w * b.mat[3].w;
        }
        return t;
    }

  private:
    Vec4 mat[4];
};

// ============================================================
//  Core data types
// ============================================================
struct Material {
    float k_ambient, k_diffuse, k_specular;
    float ns = 32.0f;              // specular exponent (shininess), MTL Ns
};

struct Light {
    Vec4 pos;
    Vec3 color;
    float intensity{};
};

struct Mesh {
    Vec4 center;
    std::vector<Vec4> vertices;
    std::vector<int> indices;
    std::vector<Vec3> colors;
    Material material;
    float bounding_radius = 0;
    static Mesh Cube(const Vec4& center, float r, const std::vector<Vec3>& colors, const Material& material);
    static Mesh Plane(const Vec4& center, float r, const std::vector<Vec3>& colors, const Material& material);
    static std::vector<Mesh> LoadOBJ(const char* path,
                                     const std::vector<Vec3>& colors = {Vec3(200, 200, 200)},
                                     const Material& material = Material{0.4f, 0.8f, 0.3f});
};

struct Tile {
    int x_start, x_end, y_start, y_end;
};
struct ID {
    int begin, end;
};
enum AA { SSAA, NOAA, FXAA, TAA };

struct Cell {
    Vec3 bg{ 0, 0, 0 }, fg{ 255, 255, 255 };
    char ch = ' ';
    bool transparent = false, has_text = false;
};

// ============================================================
//  Platform abstraction
// ============================================================
struct Stopwatch {
    std::chrono::high_resolution_clock::time_point start;
    Stopwatch()
      : start(std::chrono::high_resolution_clock::now()) {}
    [[nodiscard]] double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    void reset() { start = std::chrono::high_resolution_clock::now(); }
};

namespace platform {

inline void
console_init() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    printf("\033[?25l");
#else
    printf("\033[?25l");
    fflush(stdout);
    static bool raw = false;
    if (!raw) {
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        raw = true;
    }
#endif
}

inline void
console_get_size(int& width, int& height, bool& bias) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO c;
    GetConsoleScreenBufferInfo(h, &c);
    c.dwSize.X = c.dwMaximumWindowSize.X;
    c.dwSize.Y = c.dwMaximumWindowSize.Y;
    SetConsoleScreenBufferSize(h, c.dwSize);
    bias = c.dwSize.X % 2 != 0;
    width = c.dwSize.X / 2;
    height = c.dwSize.Y;
#else
    winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    bias = false;
    width = ws.ws_col / 2;
    height = ws.ws_row;
#endif
}

inline void
console_write(const char* data, size_t len) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD w;
    WriteConsoleA(h, data, (DWORD)len, &w, nullptr);
#else
    write(STDOUT_FILENO, data, len);
#endif
}

inline bool
kbhit() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    int f = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, f | O_NONBLOCK);
    char c;
    bool h = read(STDIN_FILENO, &c, 1) > 0;
    if (h)
        ungetc(c, stdin);
    fcntl(STDIN_FILENO, F_SETFL, f);
    return h;
#endif
}

inline int
getch() {
#ifdef _WIN32
    return _getch();
#else
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
#endif
}

inline void
get_cursor_pos(int& x, int& y) {
#ifdef _WIN32
    POINT p;
    GetCursorPos(&p);
    x = p.x;
    y = p.y;
#else
    fflush(stdout);
    write(STDOUT_FILENO, "\x1b[6n", 4);
    termios old, tmp;
    tcgetattr(STDIN_FILENO, &old);
    tmp = old;
    tmp.c_lflag &= ~(ICANON | ECHO);
    tmp.c_cc[VMIN] = 0;
    tmp.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
    char buf[32];
    int p = 0;
    while (p < 31) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;
        buf[p++] = c;
        if (c == 'R')
            break;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    buf[p] = '\0';
    int r = 0, c = 0;
    if (sscanf(buf, "\x1b[%d;%d", &r, &c) >= 2) {
        x = c / 2;
        y = r - 1;
    } else {
        x = y = 0;
    }
#endif
}

inline bool
key_down(int vk) {
#ifdef _WIN32
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
    static bool state[256] = {};
    static std::chrono::steady_clock::time_point pt[256];
    static constexpr auto HOLD = std::chrono::milliseconds(150);
    int f = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, f | O_NONBLOCK);
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        unsigned u = (unsigned char)c;
        if (u >= 32 && u <= 126) {
            if (!state[u])
                pt[u] = std::chrono::steady_clock::now();
            state[u] = true;
        }
    }
    fcntl(STDIN_FILENO, F_SETFL, f);
    unsigned u = (unsigned char)vk;
    if (state[u] && std::chrono::steady_clock::now() - pt[u] > HOLD)
        state[u] = false;
    return state[u];
#endif
}

inline void
sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

} // namespace platform

#ifndef VK_RBUTTON
#define VK_RBUTTON 0x02
#endif

// ============================================================
//  Tile partition helper
// ============================================================
inline std::vector<Tile>
partition_tiles(int width, int height, int num_tiles) {
    if (num_tiles <= 1)
        return { { 0, width, 0, height } };
    float aspect = (float)width / (float)height;
    int cols = (int)ceil(sqrt((float)num_tiles * aspect)), rows = (num_tiles + cols - 1) / cols;
    int tw = (width + cols - 1) / cols, th = (height + rows - 1) / rows;
    std::vector<Tile> tiles;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) {
            int x0 = c * tw, y0 = r * th, x1 = std::min(x0 + tw, width), y1 = std::min(y0 + th, height);
            if (x1 > x0 && y1 > y0)
                tiles.emplace_back(Tile{ x0, x1, y0, y1 });
        }
    return tiles;
}

// ============================================================
//  Screen & TileScreen
// ============================================================
class Screen {
  public:
    Screen();
    static void init();
    void update();
    virtual void set_pixel(const int& x, const int& y, const Vec3& color, const float& z);
    [[nodiscard]] virtual bool depth_test(int x, int y, float z) const;
    void clear();
    void draw();
    static void calculate_fps(double frame_time_ms);
    static int get_fps() { return fps; }
    static const std::string& get_fps_display() { return fps_display; }
    void show() const;
    void apply_ssaa();
    void apply_fxaa();
    void apply_taa();
    [[nodiscard]] int get_taa_frame_count() const { return taa_frame_count; }
    static Vec2 halton_sequence(int index);
    std::string output_buf;
    std::vector<Color> buffer;
    std::vector<float> z_buffer;
    std::vector<Color> prev_buffer;
    int width = 0, height = 0;
    bool bias = false, SSAA = false, show_fps = false;

  private:
    static int counter_t, fps;
    static double fps_min, fps_max, fps_sum;
    static int fps_count;
    static std::string fps_display;
    std::vector<Color> taa_history, aa_scratch;
    int taa_frame_count = 0;
};

class TileScreen : public Screen {
  public:
    int tile_x, tile_y, tile_w, tile_h;
    TileScreen(int gw, int gh, int tx, int ty, int tw, int th)
      : tile_x(tx)
      , tile_y(ty)
      , tile_w(tw)
      , tile_h(th) {
        width = gw;
        height = gh;
        buffer.assign(tw * th, 0);
        z_buffer.assign(tw * th, 0.0f);
    }
    void set_pixel(const int& x, const int& y, const Vec3& color, const float& z) override {
        if (x < tile_x || x >= tile_x + tile_w || y < tile_y || y >= tile_y + tile_h)
            return;
        size_t i = (size_t)(x - tile_x) + (size_t)(y - tile_y) * (size_t)tile_w;
        if (z_buffer[i] != 0 && z >= z_buffer[i])
            return;
        z_buffer[i] = z;
        buffer[i] = pack_color(color);
    }
[[nodiscard]] bool depth_test(int x, int y, float z) const override {
        if (x < tile_x || x >= tile_x + tile_w || y < tile_y || y >= tile_y + tile_h)
            return false;
        size_t i = (size_t)(x - tile_x) + (size_t)(y - tile_y) * (size_t)tile_w;
        return z_buffer[i] == 0 || z < z_buffer[i];
    }
    bool resize_if_needed(int gw, int gh, int tx, int ty, int tw, int th) {
        if ((size_t)tw * th == buffer.size() && tile_w == tw && tile_h == th) {
            tile_x = tx;
            tile_y = ty;
            width = gw;
            height = gh;
            fill(buffer.begin(), buffer.end(), 0);
            fill(z_buffer.begin(), z_buffer.end(), 0.0f);
            return true;
        }
        this->~TileScreen();
        new (this) TileScreen(gw, gh, tx, ty, tw, th);
        return false;
    }
};

// ============================================================
//  Camera
// ============================================================
class Camera {
  public:
    Camera();
    void update_from(const Screen& screen);
    void load(Screen& screen,
              const Mesh& mesh,
              const std::vector<Light>& lights,
              const Tile* clip_tile = nullptr,
              const Vec4* proj_verts = nullptr) const;
    void controller();
    void set_jitter(float jx, float jy);
    [[nodiscard]] Vec4 get_dir() const;
    void pre_project_verts(const std::vector<Mesh>& meshes, std::vector<Vec4>& out) const;
    Vec4 pos;
    float a_x = 0, a_y = 0;
    int p1x = 0, p1y = 0, p2x = 0, p2y = 0;
    Vec4 dir;
    float width = 1, height = 1, n = 1, f = 100;

  private:
    float jitter_x = 0, jitter_y = 0;
    static int near_clip(const Vec4& a, const Vec4& b, Vec4 out[2]);
    static void division(Vec4& i);
};

// ============================================================
//  Graphics: Line & Triangle
// ============================================================
class Line {
  public:
    static void draw(Screen&, const Vec2& a, const Vec2& b, const Vec3& color);

  protected:
    static void fast_draw(Screen&,
                          const Vec2& a,
                          const Vec2& b,
                          const Vec3&,
                          const Material&,
                          const std::vector<Light>&,
                          const Vec2* verts,
                          const Vec4* v,
                          const Vec4& dir,
                          const Vec4& normal,
                          const Vec3& ambient_col,
                          const Tile* scissor = nullptr);
};

class Triangle : public Line {
  public:
    static void scan_draw(Screen&,
                          const Vec2& a,
                          const Vec2& b,
                          const Vec2& c,
                          const Vec3& color,
                          const Material&,
                          const std::vector<Light>&,
                          const Vec4* v,
                          const Vec4& dir,
                          const Tile* scissor = nullptr);
    static void scan_draw(Screen&, const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& color);
};

// ============================================================
//  Transform
// ============================================================
class Transform {
  public:
    static void translate(Mesh&, const Vec4&);
    static void translate(Vec4&, const Vec4&);
    static void rotate(Mesh&, const Vec4& center, Vec4 axis, float angle);
    static void rotate(Mesh&, Vec4 axis, float angle);
    static void rotate(Vec4& dir, Vec4 axis, float angle);
    static void rotate(Vec3& point, Vec4 axis, float angle);
    static void scale(Mesh& mesh, float s);
};

// ============================================================
//  Layer2D
// ============================================================
class Layer2D {
  public:
    Layer2D(int width, int height, int z_order = 0);
    int width() const { return m_width; }
    int height() const { return m_height; }
    int z() const { return m_z; }
    void set_z(int z) { m_z = z; }
    void set_visible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }
    void clear(Vec3 bg = Vec3(0, 0, 0));
    void set_cell(int x, int y, char ch, Vec3 fg = Vec3(255, 255, 255), Vec3 bg = Vec3(0, 0, 0));
    void fill_rect(float nx, float ny, float nw, float nh, Vec3 bg);
    void draw_border(float nx, float ny, float nw, float nh, Vec3 color = Vec3(200, 200, 200));
    void draw_text(float nx,
                   float ny,
                   const std::string& text,
                   Vec3 fg = Vec3(255, 255, 255),
                   Vec3 bg = Vec3(0, 0, 0),
                   float scale = 1.0f);
    void draw_line(float nx0, float ny0, float nx1, float ny1, Vec3 color = Vec3(200, 200, 200));
    const Cell& cell(int x, int y) const { return m_cells[x + y * m_width]; }
    Cell& cell(int x, int y) { return m_cells[x + y * m_width]; }
    void composite_to(std::vector<Color>& target, int tw, int th) const;

  private:
    int m_width, m_height, m_z;
    bool m_visible = true;
    std::vector<Cell> m_cells;
};

// ============================================================
//  TUI Widgets
// ============================================================
class Widget {
  public:
    Widget(float nx, float ny, float nw, float nh)
      : m_nx(nx)
      , m_ny(ny)
      , m_nw(nw)
      , m_nh(nh) {}
    virtual ~Widget() = default;
    virtual void draw(Layer2D&) = 0;
    virtual bool handle_click(float nmx, float nmy) { return false; }
    void set_pos(float nx, float ny) {
        m_nx = nx;
        m_ny = ny;
    }
    void set_size(float nw, float nh) {
        m_nw = nw;
        m_nh = nh;
    }

  protected:
    float m_nx, m_ny, m_nw, m_nh;
};

class Label : public Widget {
  public:
    Label(float nx, float ny, const std::string& text, Vec3 fg = Vec3(200, 200, 200), float font_scale = 0.03f);
    void draw(Layer2D&) override;
    void set_text(const std::string& t) { m_text = t; }

  private:
    std::string m_text;
    Vec3 m_fg;
    float m_scale;
};

class Button : public Widget {
  public:
    Button(float nx,
           float ny,
           const std::string& text,
           std::function<void()> cb,
           Vec3 bg = Vec3(60, 60, 60),
           Vec3 fg = Vec3(220, 220, 220),
           float font_scale = 0.03f);
    void draw(Layer2D&) override;
    bool handle_click(float nmx, float nmy) override;

  private:
    std::string m_text;
    std::function<void()> m_cb;
    Vec3 m_bg, m_fg;
    float m_scale;
};

class TUI {
  public:
    void add(std::unique_ptr<Widget> w) { m_widgets.push_back(std::move(w)); }
    void draw(Layer2D& l) {
        for (auto& w : m_widgets)
            w->draw(l);
    }
    bool handle_click(float nmx, float nmy) {
        for (auto& w : m_widgets)
            if (w->handle_click(nmx, nmy))
                return true;
        return false;
    }

  private:
    std::vector<std::unique_ptr<Widget>> m_widgets;
};

// ============================================================
//  Renderer — main engine API
// ============================================================
class Renderer {
  public:
    Renderer();
    ~Renderer();
    void launch();
    void shutdown();
    void update();
    void controller();

    void set_camera_pos(const Vec4& pos);
    void set_aa(AA mode);
    void toggle_fps();

    ID add_meshes(const std::vector<Mesh>& meshes);
    void remove_meshes(ID id);
    void clear_meshes();
    std::vector<Mesh*> get_meshes(ID id);
    size_t mesh_count() const { return meshes.size(); }

    ID add_lights(const std::vector<Light>& lights);
    void remove_lights(ID id);
    void clear_lights();
    std::vector<Light*> get_lights(ID id);
    size_t light_count() const { return lights.size(); }

    Screen* get_screen() { return &screen; }
    Camera* get_camera() { return &camera; }

  private:
    void prepare_frame();
    void composite_frame();
    void render_frame();
    void render_tile(const Tile& tile, TileScreen& ts);
    void worker_loop(int thread_id);
    void frustum_cull();
    void hiz_clear();
    bool hiz_test(float cx, float cy, float cz, float radius) const;
    void hiz_update(int x, int y, int w, int h, const std::vector<float>& z_buf, int zw);
    void composite_layers();

    Camera camera;
    Screen screen;
    std::vector<Mesh> meshes;
    std::vector<Light> lights;
    int mesh_num = 0, light_num = 0;
    std::vector<Mesh> frame_meshes;
    std::vector<Light> frame_lights;
    std::vector<Vec4> frame_vert_proj;
    std::vector<bool> mesh_visible;
    std::vector<float> hiz_buffer;
    int hiz_w = 0, hiz_h = 0;
    Layer2D overlay{ 0, 0, 10 };
    std::vector<std::thread> workers;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> current_frame{ 0 }, workers_done{ 0 };
    bool running{ false };
    int num_threads{ 1 };
    std::vector<Tile> tiles;
    std::atomic<int> tile_index{ 0 };
    AA aa_mode{ NOAA };
};

// ============================================================
//  RubikCube demo helper
// ============================================================
class RubikCube {
  public:
    explicit RubikCube(float length);
    static void control(Vec3* blocks, const std::vector<Mesh*>& meshes);
    Vec3 block_indices[27];
    std::vector<Mesh> meshes;

  private:
    static Mesh gen_block(const Vec4& pos, float len);
};

// ============================================================
//  ── IMPLEMENTATION ────────────────────────────────────────
//  Define RENDER_ENGINE_IMPLEMENTATION before including this
//  header in exactly ONE translation unit.
// ============================================================
#ifdef RENDER_ENGINE_IMPLEMENTATION

// Bring commonly-used std names into scope for brevity
using std::max;
using std::min;
using std::mutex;
using std::swap;
using std::thread;
using std::unique_lock;
using std::vector;



// ── 5×5 bitmap font (from saveData, 95 chars) ─────────────────
static const uint8_t s_font3x5[95*5] = {
    0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x00,0x40,0xA0,0xA0,0x00,0x00,0x00,0xA0,0xE0,0xA0,0xE0,0xA0,
    0x40,0xE0,0x40,0xE0,0x40,0x80,0x20,0x40,0x80,0x20,0x40,0x20,0xC0,0xA0,0x40,0x40,0x40,0x00,0x00,0x00,
    0x40,0x80,0x80,0x80,0x40,0x40,0x20,0x20,0x20,0x40,0x00,0x40,0xA0,0x40,0x00,0x00,0x40,0xE0,0x40,0x00,
    0x00,0x00,0x00,0x40,0x40,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x20,0x40,0x80,0x00,
    0xE0,0xA0,0xA0,0xA0,0xE0,0xC0,0x40,0x40,0x40,0xE0,0xE0,0x20,0xE0,0x80,0xE0,0xE0,0x20,0x60,0x20,0xE0,
    0xA0,0xA0,0xE0,0x20,0x20,0xE0,0x80,0xE0,0x20,0xE0,0xE0,0x80,0xE0,0xA0,0xE0,0xE0,0x20,0x20,0x20,0x20,
    0xE0,0xA0,0xE0,0xA0,0xE0,0xE0,0xA0,0xE0,0x20,0xE0,0x00,0x40,0x00,0x40,0x00,0x00,0x40,0x00,0x40,0x40,
    0x20,0x40,0x80,0x40,0x20,0x00,0xE0,0x00,0xE0,0x00,0x80,0x40,0x20,0x40,0x80,0xE0,0x20,0x40,0x00,0x40,
    0xE0,0xA0,0xA0,0x80,0xE0,0x40,0xA0,0xE0,0xA0,0xA0,0xC0,0xA0,0xC0,0xA0,0xE0,0xE0,0x80,0x80,0x80,0xE0,
    0xC0,0xA0,0xA0,0xA0,0xC0,0xE0,0x80,0xE0,0x80,0xE0,0xE0,0x80,0xE0,0x80,0x80,0xE0,0x80,0xA0,0xA0,0xE0,
    0xA0,0xA0,0xE0,0xA0,0xA0,0xE0,0x40,0x40,0x40,0xE0,0xE0,0x40,0x40,0x40,0xC0,0x80,0xE0,0x80,0xC0,0xA0,
    0x80,0x80,0x80,0x80,0xE0,0xA0,0xE0,0xE0,0xA0,0xA0,0xE0,0xA0,0xA0,0xA0,0xA0,0x40,0xA0,0xA0,0xA0,0x40,
    0xE0,0xA0,0xE0,0x80,0x80,0xE0,0xA0,0xE0,0x20,0x20,0x80,0xE0,0x80,0x80,0x80,0xE0,0x80,0x40,0x20,0xE0,
    0xE0,0x40,0x40,0x40,0x40,0xA0,0xA0,0xA0,0xA0,0xE0,0xA0,0xA0,0xA0,0xA0,0x40,0xA0,0xA0,0xE0,0xE0,0xA0,
    0xA0,0xA0,0x40,0xA0,0xA0,0xA0,0xA0,0xA0,0x40,0x40,0xE0,0x20,0x40,0x80,0xE0,0xC0,0x80,0x80,0x80,0xC0,
    0x00,0x80,0x40,0x20,0x00,0x60,0x20,0x20,0x20,0x60,0x40,0xA0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,
    0x40,0x20,0x00,0x00,0x00,0xE0,0xA0,0xE0,0xA0,0xA0,0x80,0x80,0xE0,0xA0,0xE0,0xE0,0xA0,0x80,0x80,0xE0,
    0x20,0x20,0xE0,0xA0,0xE0,0xE0,0xA0,0xE0,0x80,0xE0,0xE0,0x80,0xC0,0x80,0x80,0xE0,0xA0,0xE0,0x20,0x60,
    0x80,0x80,0xE0,0xA0,0xA0,0x40,0x00,0x40,0x40,0x40,0x40,0x00,0x40,0x40,0xC0,0x80,0xA0,0xC0,0xA0,0xA0,
    0x40,0x40,0x40,0x40,0x60,0x00,0xA0,0xE0,0xA0,0xA0,0x00,0xE0,0xA0,0xA0,0xA0,0x00,0x40,0xA0,0xA0,0x40,
    0x00,0xE0,0xA0,0xE0,0x80,0x00,0xE0,0xA0,0xE0,0x20,0x00,0x80,0xC0,0x80,0x80,0x60,0x80,0x40,0x20,0xC0,
    0x40,0xE0,0x40,0x40,0x60,0x00,0xA0,0xA0,0xA0,0xE0,0x00,0xA0,0xA0,0xA0,0x40,0x00,0xA0,0xA0,0xE0,0xA0,
    0x00,0xA0,0x40,0xA0,0x00,0xA0,0xA0,0xE0,0x20,0xE0,0x00,0xE0,0x40,0x80,0xE0,0x20,0x40,0xC0,0x40,0x20,
    0x40,0x40,0x40,0x40,0x40,0x80,0x40,0x60,0x40,0x80,0x00,0x20,0xE0,0x80,0x00,
};

// ── auto_color helper ──────────────────────────────────────────
static std::vector<Vec3>
auto_color(const std::vector<Vec3>& colors, int size) {
    std::vector<Vec3> c;
    c.resize(size);
    for (int i = 0; i < size; i++) {
        if (colors.size() == 1)
            c[i] = colors[0];
        else if (colors.size() * 3 == size)
            c[i] = colors[i / 3];
        else if (colors.size() * 6 == size)
            c[i] = colors[i / 6];
    }
    return c;
}

// ── Mesh implementation ────────────────────────────────────────
Mesh
Mesh::Cube(const Vec4& center, float r, const std::vector<Vec3>& colors, const Material& material) {
    std::vector<Vec4> verts = {
        { center.x - r, center.y - r, center.z - r, 1 }, { center.x + r, center.y - r, center.z - r, 1 },
        { center.x + r, center.y + r, center.z - r, 1 }, { center.x - r, center.y + r, center.z - r, 1 },
        { center.x - r, center.y - r, center.z + r, 1 }, { center.x + r, center.y - r, center.z + r, 1 },
        { center.x + r, center.y + r, center.z + r, 1 }, { center.x - r, center.y + r, center.z + r, 1 },
    };
    std::vector<int> idx = { 0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5,
                             4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 1, 0, 4, 4, 5, 1 };
    return { center, verts, idx, auto_color(colors, (int)idx.size()), material, 1.7320508f * r };
}
Mesh
Mesh::Plane(const Vec4& center, float r, const std::vector<Vec3>& colors, const Material& m) {
    std::vector<Vec4> verts = { { center.x - r, center.y, center.z - r, 1 },
                                { center.x + r, center.y, center.z - r, 1 },
                                { center.x + r, center.y, center.z + r, 1 },
                                { center.x - r, center.y, center.z + r, 1 } };
    std::vector<int> idx = { 0, 1, 2, 2, 3, 0 };
    return { center, verts, idx, auto_color(colors, (int)idx.size()), m, 1.4142136f * r };
}

// ── OBJ + MTL loader ─────────────────────────────────────────────
//  Parses Wavefront .obj files with optional .mtl material library.
//  - Handles v / f (tri & quad), o/g grouping, mtllib / usemtl
//  - MTL: newmtl, Ka, Kd, Ks, Ns, d/Tr
//  - Returns one Mesh per (o/g) × (usemtl) combination.
//  - Vertex indices: 1-based → 0-based, negative = relative.
//
//  Usage:
//    auto meshes = Mesh::LoadOBJ("model.obj");
//    ID id = renderer.add_meshes(meshes);
//
struct MtlEntry {
    std::string name;
    Vec3    diffuse = Vec3(200, 200, 200);
    Material mat    = Material{0.4f, 0.8f, 0.3f};
};

// ── Parse a .mtl file ──────────────────────────────────────────
static std::vector<MtlEntry>
load_mtl(const char* path) {
    std::vector<MtlEntry> result;
    FILE* fp = fopen(path, "r");
    if (!fp) return result;

    char line[4096];
    MtlEntry cur;
    std::string cur_name;

    auto flush = [&]() {
        if (!cur_name.empty()) {
            cur.name = cur_name;
            result.push_back(cur);
            cur_name.clear();
        }
    };

    while (fgets(line, sizeof(line), fp)) {
        char type[16];
        if (sscanf(line, "%15s", type) != 1) continue;
        if (type[0] == '#') continue;

        if (strcmp(type, "newmtl") == 0) {
            flush();
            char buf[256] = {};
            sscanf(line, "newmtl %255s", buf);
            cur_name = buf;
            cur = MtlEntry();
        } else if (strcmp(type, "Ka") == 0) {
            float r, g, b;
            if (sscanf(line, "Ka %f %f %f", &r, &g, &b) >= 3)
                cur.mat.k_ambient = (r + g + b) / 3.0f * 255.0f;
        } else if (strcmp(type, "Kd") == 0) {
            float r, g, b;
            if (sscanf(line, "Kd %f %f %f", &r, &g, &b) >= 3)
                cur.diffuse = Vec3((int)(r * 255), (int)(g * 255), (int)(b * 255));
        } else if (strcmp(type, "Ks") == 0) {
            float r, g, b;
            if (sscanf(line, "Ks %f %f %f", &r, &g, &b) >= 3)
                cur.mat.k_specular = (r + g + b) / 3.0f;
        } else if (strcmp(type, "Ns") == 0) {
            float ns;
            if (sscanf(line, "Ns %f", &ns) == 1)
                cur.mat.ns = ns;
        }
        // d / Tr (transparency) — not yet supported by the renderer
    }
    flush();
    fclose(fp);
    return result;
}

static int
obj_vert_idx(int i, int count) {
    if (i > 0) return i - 1;
    if (i < 0) return count + i;
    return 0;
}

static std::string
obj_dir(const char* path) {
    std::string p(path);
    auto pos = p.find_last_of("/\\");
    if (pos != std::string::npos)
        p.resize(pos + 1);
    else
        p.clear();
    return p;
}

static const MtlEntry*
obj_find_mtl(const std::vector<MtlEntry>& lib, const char* name) {
    for (auto& e : lib)
        if (e.name == name) return &e;
    return nullptr;
}

std::vector<Mesh>
Mesh::LoadOBJ(const char* path, const std::vector<Vec3>& colors, const Material& material) {
    std::vector<Mesh> result;
    FILE* fp = fopen(path, "r");
    if (!fp) return result;

    // ── Pass 1: load referenced .mtl files ──────────────────────
    std::vector<MtlEntry> mtl_lib;
    std::string base = obj_dir(path);
    {
        char line[4096];
        while (fgets(line, sizeof(line), fp)) {
            char type[16];
            if (sscanf(line, "%15s", type) != 1) continue;
            if (strcmp(type, "mtllib") == 0) {
                char mtl_path[1024];
                if (sscanf(line, "mtllib %1023s", mtl_path) == 1) {
                    auto loaded = load_mtl((base + mtl_path).c_str());
                    mtl_lib.insert(mtl_lib.end(), loaded.begin(), loaded.end());
                }
            }
        }
    }
    rewind(fp);

    // ── Pass 2: parse geometry ─────────────────────────────────
    std::vector<Vec4> verts;
    verts.reserve(4096);
    std::vector<int> indices;

    // Current material state (falls back to function params)
    Vec3 cur_color  = colors.empty() ? Vec3(200, 200, 200) : colors[0];
    Material cur_mat = material;
    bool has_content = false;

    // Vertex dedup state (persists across flush calls)
    std::vector<int> _vgen, _vremap;
    int _vcur = 0;

    auto flush = [&]() {
        if (indices.empty()) return;
        Mesh mesh;

        // Only keep vertices referenced by indices → no bloat with many groups
        ++_vcur;
        std::vector<Vec4> local_verts;
        std::vector<int>  local_idx;
        local_verts.reserve(indices.size() / 3);
        local_idx.reserve(indices.size());

        for (int old_idx : indices) {
            if ((size_t)old_idx >= _vgen.size()) {
                _vgen.resize((size_t)old_idx + 1, 0);
                _vremap.resize((size_t)old_idx + 1, 0);
            }
            if (_vgen[old_idx] != _vcur) {
                _vgen[old_idx] = _vcur;
                _vremap[old_idx] = (int)local_verts.size();
                local_verts.push_back(verts[old_idx]);
            }
            local_idx.push_back(_vremap[old_idx]);
        }

        mesh.vertices = std::move(local_verts);
        mesh.indices  = std::move(local_idx);
        mesh.colors.assign(mesh.indices.size(), cur_color);
        mesh.material = cur_mat;

        // centroid
        Vec4 c{0, 0, 0, 0};
        for (auto& v : mesh.vertices) c = c + v;
        if (!mesh.vertices.empty())
            c = c * (1.0f / (float)mesh.vertices.size());
        c.w = 1;
        mesh.center = c;

        float r = 0;
        for (auto& v : mesh.vertices) {
            float dx = v.x - c.x, dy = v.y - c.y, dz = v.z - c.z;
            float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (d > r) r = d;
        }
        mesh.bounding_radius = r;

        result.push_back(std::move(mesh));
        indices.clear();
        has_content = false;
    };

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        char type[16];
        if (sscanf(line, "%15s", type) != 1) continue;
        if (type[0] == '#') continue;

        if (strcmp(type, "v") == 0 && type[1] == '\0') {
            float x, y, z;
            if (sscanf(line, "v %f %f %f", &x, &y, &z) >= 3)
                verts.push_back({x, y, z, 1});
        } else if (strcmp(type, "usemtl") == 0) {
            if (has_content) flush();
            char name[256] = {};
            sscanf(line, "usemtl %255s", name);
            auto* entry = obj_find_mtl(mtl_lib, name);
            if (entry) {
                cur_color = entry->diffuse;
                cur_mat   = entry->mat;
            }
        } else if (strcmp(type, "o") == 0 || strcmp(type, "g") == 0) {
            if (has_content) flush();
        } else if (strcmp(type, "f") == 0) {
            // Parse face — extract vertex indices (ignore vt/vn)
            int v_idx[4] = {0}, count = 0;
            const char* p = line + 1;
            while (*p && count < 4) {
                while (*p == ' ' || *p == '\t') ++p;
                if (*p == '\0' || *p == '\n' || *p == '\r') break;
                int sign = 1, val = 0;
                if (*p == '-') { sign = -1; ++p; }
                else if (*p == '+') ++p;
                while (*p >= '0' && *p <= '9') {
                    val = val * 10 + (*p - '0');
                    ++p;
                }
                v_idx[count] = val * sign;
                ++count;
                while (*p && *p != ' ' && *p != '\t') ++p;  // skip /vt/vn tail
            }
            if (count < 3) continue;

            int i0 = obj_vert_idx(v_idx[0], (int)verts.size());
            int i1 = obj_vert_idx(v_idx[1], (int)verts.size());
            int i2 = obj_vert_idx(v_idx[2], (int)verts.size());
            if (i0 < 0 || i0 >= (int)verts.size() || i1 < 0 || i1 >= (int)verts.size() ||
                i2 < 0 || i2 >= (int)verts.size())
                continue;
            // Flip winding → renderer expects CW in screen space (y-down)
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            has_content = true;

            if (count >= 4) {  // quad → fan triangulation
                int i3 = obj_vert_idx(v_idx[3], (int)verts.size());
                if (i3 >= 0 && i3 < (int)verts.size()) {
                    indices.push_back(i0);
                    indices.push_back(i2);
                    indices.push_back(i3);
                }
            }
        }
    }
    flush();
    fclose(fp);
    return result;
}
int Screen::counter_t = 0;
int Screen::fps = 0;
double Screen::fps_min = 1e9;
double Screen::fps_max = 0;
double Screen::fps_sum = 0;
int Screen::fps_count = 0;
std::string Screen::fps_display = "FPS: 0/0";

Screen::Screen() {
    width = height = 0;
    bias = false;
    SSAA = false;
    show_fps = false;
}
void
Screen::init() {
    platform::console_init();
}
void
Screen::update() {
    platform::console_get_size(width, height, bias);
}

void
Screen::set_pixel(const int& x, const int& y, const Vec3& color, const float& z) {
    if (x < 0 || x >= width || y < 0 || y >= height)
        return;
    size_t i = (size_t)x + (size_t)y * (size_t)width;
    if (z_buffer[i] != 0 && z >= z_buffer[i])
        return;
    z_buffer[i] = z;
    buffer[i] = pack_color(color);
}
bool
Screen::depth_test(int x, int y, float z) const {
    if (x < 0 || x >= width || y < 0 || y >= height)
        return false;
    size_t i = (size_t)x + (size_t)y * (size_t)width;
    return z_buffer[i] == 0 || z < z_buffer[i];
}

static void
append_uint(std::string& b, int n) {
    if (n >= 1000) {
        b += (char)('0' + n / 1000);
        b += (char)('0' + (n / 100) % 10);
        b += (char)('0' + (n / 10) % 10);
        b += (char)('0' + n % 10);
    } else if (n >= 100) {
        b += (char)('0' + n / 100);
        b += (char)('0' + (n / 10) % 10);
        b += (char)('0' + n % 10);
    } else if (n >= 10) {
        b += (char)('0' + n / 10);
        b += (char)('0' + n % 10);
    } else {
        b += (char)('0' + n);
    }
}
static void
append_spans(std::string& b, int cnt) {
    if (cnt <= 0)
        return;
    if (cnt < 6)
        b.append((size_t)cnt, ' ');
    else {
        b += ' ';
        b += "\x1b[";
        append_uint(b, cnt - 1);
        b += 'b';
    }
}

void
Screen::clear() {
    buffer.assign(width * height, 0);
    z_buffer.assign(width * height, 0);
}

void
Screen::draw() {
    const int w = width, h = height;
    const size_t pc = (size_t)w * h;
    output_buf.reserve((size_t)w * h * 30 + 1024);
    if (prev_buffer.size() != pc) {
        prev_buffer.assign(pc, 0);
        output_buf = "\033[3J\033[H";
        for (int y = 0; y < h; y++) {
            int x = 0;
            while (x < w) {
                Color c = buffer[x + y * w];
                int run = 1;
                while (x + run < w) {
                    if (buffer[x + run + y * w] != c)
                        break;
                    run++;
                }
                output_buf += "\033[48;2;";
                append_uint(output_buf, (c >> 16) & 0xFF);
                output_buf += ';';
                append_uint(output_buf, (c >> 8) & 0xFF);
                output_buf += ';';
                append_uint(output_buf, c & 0xFF);
                output_buf += 'm';
                append_spans(output_buf, run * 2);
                x += run;
            }
            if (bias)
                output_buf += "\033[m ";
        }
        prev_buffer = buffer;
        return;
    }
    output_buf = "\033[H";
    for (int y = 0; y < h; y++) {
        bool dirty = false;
        int x = 0;
        while (x < w) {
            size_t idx = (size_t)x + (size_t)y * (size_t)w;
            Color cur = buffer[idx], prv = prev_buffer[idx];
            if (cur == prv) {
                x++;
                continue;
            }
            if (!dirty) {
                dirty = true;
                output_buf += "\033[";
                append_uint(output_buf, y + 1);
                output_buf += ";1H";
            }
            int run = 1;
            while (x + run < w) {
                if (buffer[(size_t)(x + run) + (size_t)y * (size_t)w] != cur)
                    break;
                run++;
            }
            output_buf += "\033[";
            append_uint(output_buf, x * 2 + 1);
            output_buf += "G\033[48;2;";
            append_uint(output_buf, (cur >> 16) & 0xFF);
            output_buf += ';';
            append_uint(output_buf, (cur >> 8) & 0xFF);
            output_buf += ';';
            append_uint(output_buf, cur & 0xFF);
            output_buf += 'm';
            append_spans(output_buf, run * 2);
            x += run;
        }
    }
    prev_buffer = buffer;
}

void
Screen::calculate_fps(double ft) {
    static int bc = 0;
    static double bm = 0;
    bm += ft;
    if (++bc >= 10) {
        double avg = 1000.0 / (bm / 10.0);
        if (avg < fps_min)
            fps_min = avg;
        if (avg > fps_max)
            fps_max = avg;
        static auto lu = std::chrono::high_resolution_clock::now();
        auto n = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double>(n - lu).count() >= 1.0) {
            char buf[64];
            snprintf(buf, sizeof(buf), "FPS: %.0f/%.0f", fps_min, fps_max);
            fps_display = buf;
            fps_min = 1e9;
            fps_max = 0;
            lu = n;
        }
        bm = 0;
        bc = 0;
    }
}

void
Screen::apply_ssaa() {
    if (width < 2 || height < 2)
        return;
    int nw = width / 2, nh = height / 2;
    aa_scratch.assign(nw * nh, 0);
    for (int y = 0; y < nh; y++)
        for (int x = 0; x < nw; x++) {
            size_t s = (size_t)(x * 2) + (size_t)(y * 2) * (size_t)width;
            auto up = [&](size_t o) {
                auto v = unpack_color(buffer[o]);
                return Vec3(v.x, v.y, v.z);
            };
            Vec3 a0 = up(s), a1 = up(s + 1), a2 = up(s + width), a3 = up(s + width + 1);
            aa_scratch[(size_t)y * (size_t)nw + (size_t)x] = pack_color(
              Vec3((a0.x + a1.x + a2.x + a3.x) / 4, (a0.y + a1.y + a2.y + a3.y) / 4, (a0.z + a1.z + a2.z + a3.z) / 4));
        }
    buffer.swap(aa_scratch);
    z_buffer.assign(nw * nh, 0);
    width = nw;
    height = nh;
}

void
Screen::apply_fxaa() {
    if (buffer.empty())
        return;
    aa_scratch = buffer;
    for (int y = 1; y < height - 1; y++)
        for (int x = 1; x < width - 1; x++) {
            size_t i = (size_t)x + (size_t)y * (size_t)width;
            auto uv = [&](size_t o) { return unpack_color(buffer[o]); };
            Vec3 cp = uv(i), lp = uv((x - 1) + y * width), rp = uv((x + 1) + y * width);
            Vec3 tp = uv(x + (y - 1) * width), bp = uv(x + (y + 1) * width);
            auto lum = [&](const Vec3& v) { return 0.299f * v.x + 0.587f * v.y + 0.114f * v.z; };
            float lc[5] = { lum(cp), lum(lp), lum(rp), lum(tp), lum(bp) };
            float mn = lc[0], mx = lc[0];
            for (int j = 1; j < 5; j++) {
                if (lc[j] < mn)
                    mn = lc[j];
                if (lc[j] > mx)
                    mx = lc[j];
            }
            if (mx - mn < 0.1f * mx)
                continue;
            Vec3 res;
            if (abs(lc[1] - lc[2]) > abs(lc[3] - lc[4])) {
                Vec3 ap = uv(x + max(y - 1, 0) * width), bep = uv(x + min(y + 1, height - 1) * width);
                res = Vec3((int)lerp((float)cp.x, (ap.x + bep.x) * 0.5f, 0.5f),
                           (int)lerp((float)cp.y, (ap.y + bep.y) * 0.5f, 0.5f),
                           (int)lerp((float)cp.z, (ap.z + bep.z) * 0.5f, 0.5f));
            } else {
                Vec3 lep = uv(max(x - 1, 0) + y * width), rip = uv(min(x + 1, width - 1) + y * width);
                res = Vec3((int)lerp((float)cp.x, (lep.x + rip.x) * 0.5f, 0.5f),
                           (int)lerp((float)cp.y, (lep.y + rip.y) * 0.5f, 0.5f),
                           (int)lerp((float)cp.z, (lep.z + rip.z) * 0.5f, 0.5f));
            }
            aa_scratch[i] = pack_color(Vec3(clamp(res.x, 0, 255), clamp(res.y, 0, 255), clamp(res.z, 0, 255)));
        }
    buffer.swap(aa_scratch);
}

void
Screen::apply_taa() {
    if (buffer.empty())
        return;
    size_t pc = (size_t)width * height;
    if (taa_history.size() != pc) {
        taa_history = buffer;
        return;
    }
    aa_scratch.assign(pc, 0);
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++) {
            size_t i = (size_t)x + (size_t)y * (size_t)width;
            Vec3 cur = unpack_color(buffer[i]), hist = unpack_color(taa_history[i]);
            int mi[3] = { 255, 255, 255 }, ma[3] = { 0, 0, 0 };
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                        continue;
                    Vec3 n = unpack_color(buffer[nx + ny * width]);
                    if (n.x < mi[0])
                        mi[0] = n.x;
                    if (n.x > ma[0])
                        ma[0] = n.x;
                    if (n.y < mi[1])
                        mi[1] = n.y;
                    if (n.y > ma[1])
                        ma[1] = n.y;
                    if (n.z < mi[2])
                        mi[2] = n.z;
                    if (n.z > ma[2])
                        ma[2] = n.z;
                }
            Vec3 cl;
            cl.x = clamp(hist.x, mi[0], ma[0]);
            cl.y = clamp(hist.y, mi[1], ma[1]);
            cl.z = clamp(hist.z, mi[2], ma[2]);
            aa_scratch[i] = pack_color(Vec3((int)lerp((float)cl.x, (float)cur.x, 0.5f),
                                            (int)lerp((float)cl.y, (float)cur.y, 0.5f),
                                            (int)lerp((float)cl.z, (float)cur.z, 0.5f)));
        }
    buffer.swap(aa_scratch);
    taa_history = buffer;
    taa_frame_count++;
}

Vec2
Screen::halton_sequence(int idx) {
    if (idx < 1)
        idx = 1;
    float f = 1.0f, x = 0.0f;
    int i = idx;
    while (i > 0) {
        f *= 0.5f;
        x += (float)(i & 1) * f;
        i >>= 1;
    }
    f = 1.0f;
    float y = 0.0f;
    i = idx;
    while (i > 0) {
        f /= 3.0f;
        y += (float)(i % 3) * f;
        i /= 3;
    }
    x -= 0.5f;
    y -= 0.5f;
    return Vec2((int)(x * 1000000), (int)(y * 1000000));
}

void
Screen::show() const {
    platform::console_write("\x1b[?2026h", 8);
    platform::console_write(output_buf.data(), output_buf.size());
    platform::console_write("\x1b[?2026l", 8);
}

// ── Camera implementation ──────────────────────────────────────
Camera::Camera() {
    width = height = 1;
    n = 1;
    f = 100;
    pos = Vec4(0, 0, 0, 1);
    a_x = a_y = 0;
    platform::get_cursor_pos(p1x, p1y);
    p2x = p1x;
    p2y = p1y;
}
void
Camera::update_from(const Screen& s) {
    width = s.width / (float)s.height;
}

void
Camera::pre_project_verts(const std::vector<Mesh>& meshes, std::vector<Vec4>& out) const {
    out.clear();
    for (auto& m : meshes)
        for (auto& s : m.vertices)
            out.push_back({ s.x * n * 2 / width, -s.y * n * 2, (s.z * (f + n) - 2 * n * f) / (f - n), s.z });
}

void
Camera::load(Screen& screen,
             const Mesh& mesh,
             const std::vector<Light>& lights,
             const Tile* clip_tile,
             const Vec4* proj_verts) const {
    int vn = (int)mesh.vertices.size();
    std::vector<Vec4> vbuf(vn);
    Vec4* vbuf_ptr = vbuf.data();
    if (proj_verts)
        for (int i = 0; i < vn; i++)
            vbuf_ptr[i] = proj_verts[i];
    else
        for (int i = 0; i < vn; i++) {
            auto& s = mesh.vertices[i];
            vbuf_ptr[i] = { s.x * n * 2 / width, -s.y * n * 2, (s.z * (f + n) - 2 * n * f) / (f - n), s.z };
        }
    float sx = 0.5f * screen.width, sy = 0.5f * screen.height;
    if ((jitter_x != 0 || jitter_y != 0) && vn > 0) {
        float jx = jitter_x * 1.2f / screen.width, jy = jitter_y * 1.2f / screen.height;
        for (int i = 0; i < vn; i++)
            if (vbuf_ptr[i].w > 0.001f) {
                vbuf_ptr[i].x += jx * vbuf_ptr[i].w;
                vbuf_ptr[i].y += jy * vbuf_ptr[i].w;
            }
    }
    if (clip_tile && vn > 0) {
        float mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
        for (int i = 0; i < vn; i++) {
            if (vbuf_ptr[i].z < -vbuf_ptr[i].w)
                continue;
            float iw = 1 / vbuf_ptr[i].w, px = vbuf_ptr[i].x * iw, py = vbuf_ptr[i].y * iw, scx = (px + 1) * sx,
                  scy = (py + 1) * sy;
            if (scx < mnx)
                mnx = scx;
            if (scx > mxx)
                mxx = scx;
            if (scy < mny)
                mny = scy;
            if (scy > mxy)
                mxy = scy;
        }
        if (mxx < clip_tile->x_start || mnx >= clip_tile->x_end || mxy < clip_tile->y_start || mny >= clip_tile->y_end)
            return;
    }
    for (int ti = 0; ti + 2 < (int)mesh.indices.size(); ti += 3) {
        Vec3 color = mesh.colors[ti];
        int ia = mesh.indices[ti], ib = mesh.indices[ti + 1], ic = mesh.indices[ti + 2];
        if (vbuf_ptr[ia].z < -vbuf_ptr[ia].w && vbuf_ptr[ib].z < -vbuf_ptr[ib].w && vbuf_ptr[ic].z < -vbuf_ptr[ic].w)
            continue;
        Vec4 cv[6];
        int cvn;
        if (vbuf_ptr[ia].z >= -vbuf_ptr[ia].w && vbuf_ptr[ib].z >= -vbuf_ptr[ib].w && vbuf_ptr[ic].z >= -vbuf_ptr[ic].w) {
            cv[0] = vbuf_ptr[ia];
            cv[1] = vbuf_ptr[ib];
            cv[2] = vbuf_ptr[ic];
            cvn = 3;
        } else {
            cvn = 0;
            cvn += near_clip(vbuf_ptr[ia], vbuf_ptr[ib], cv + cvn);
            cvn += near_clip(vbuf_ptr[ib], vbuf_ptr[ic], cv + cvn);
            cvn += near_clip(vbuf_ptr[ic], vbuf_ptr[ia], cv + cvn);
        }
        if (cvn < 3)
            continue;
        Vec4 bd[6];
        for (int v = 0; v < cvn; v++) {
            bd[v] = cv[v];
            bd[v].x *= width / n / 2;
            bd[v].y /= n * -2;
            bd[v].z = bd[v].w;
            bd[v].w = 1;
        }
        division(cv[0]);
        division(cv[1]);
        float ox = cv[0].x * sx, oy = cv[0].y * sy;
        for (int j = 1; j + 1 < cvn; j++) {
            division(cv[j + 1]);
            if (cv[0].x < 0 && cv[j].x < 0 && cv[j + 1].x < 0)
                continue;
            if (cv[0].x >= 2 && cv[j].x >= 2 && cv[j + 1].x >= 2)
                continue;
            if ((cv[j].x - cv[0].x) * (cv[j + 1].y - cv[0].y) - (cv[j].y - cv[0].y) * (cv[j + 1].x - cv[0].x) > 0)
                continue;
            float p0x = ox, p0y = oy, p1x = cv[j].x * sx, p1y = cv[j].y * sy, p2x = cv[j + 1].x * sx,
                  p2y = cv[j + 1].y * sy;
            if (clip_tile) {
                int xp[3] = { (int)p0x, (int)p1x, (int)p2x }, yp[3] = { (int)p0y, (int)p1y, (int)p2y };
                int mn = xp[0], mx = xp[0] + 1;
                for (int v = 1; v < 3; v++) {
                    if (xp[v] < mn)
                        mn = xp[v];
                    if (xp[v] + 1 > mx)
                        mx = xp[v] + 1;
                }
                int mny = yp[0], mxy = yp[0] + 1;
                for (int v = 1; v < 3; v++) {
                    if (yp[v] < mny)
                        mny = yp[v];
                    if (yp[v] + 1 > mxy)
                        mxy = yp[v] + 1;
                }
                if (mx < clip_tile->x_start || mn >= clip_tile->x_end || mxy < clip_tile->y_start ||
                    mny >= clip_tile->y_end)
                    continue;
            }
            Vec4 va[3] = { bd[0], bd[j], bd[j + 1] };
            Triangle::scan_draw(screen,
                                Vec2((int)p0x, (int)p0y),
                                Vec2((int)p1x, (int)p1y),
                                Vec2((int)p2x, (int)p2y),
                                color,
                                mesh.material,
                                lights,
                                va,
                                dir,
                                clip_tile);
        }
    }
}
void
Camera::set_jitter(float jx, float jy) {
    jitter_x = jx;
    jitter_y = jy;
}
int
Camera::near_clip(const Vec4& a, const Vec4& b, Vec4 out[2]) {
    if (a.z >= -a.w) {
        if (b.z >= -b.w) {
            out[0] = b;
            return 1;
        }
        float da = a.z + a.w, db = b.z + b.w;
        out[0] = b * (da / (da - db)) - a * (db / (da - db));
        return 1;
    }
    if (b.z >= -b.w) {
        float da = a.z + a.w, db = b.z + b.w;
        out[0] = a * (db / (db - da)) - b * (da / (db - da));
        out[1] = b;
        return 2;
    }
    return 0;
}
void
Camera::division(Vec4& i) {
    if (fabs(i.w) > 0.00001f) {
        i.x = i.x / i.w + 1;
        i.y = i.y / i.w + 1;
    }
}

void
Camera::controller() {
    // Delta time (frame-rate independent movement)
    static auto last_t = std::chrono::high_resolution_clock::now();
    auto now_t = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now_t - last_t).count();
    if (dt > 0.1f) dt = 0.1f;  // cap for large pauses
    last_t = now_t;

    constexpr float base_spd = 5.0f;
    float spd = base_spd * dt;
    if (platform::key_down('W'))
        Transform::translate(pos, Vec4(sin(a_x), 0, cos(a_x), 0) * spd);
    if (platform::key_down('S'))
        Transform::translate(pos, Vec4(-sin(a_x), 0, -cos(a_x), 0) * spd);
    if (platform::key_down('A'))
        Transform::translate(pos, Vec4(-cos(a_x), 0, sin(a_x), 0) * spd);
    if (platform::key_down('D'))
        Transform::translate(pos, Vec4(cos(a_x), 0, -sin(a_x), 0) * spd);
    if (platform::key_down('Q'))
        Transform::translate(pos, Vec4(0, 1, 0, 0) * spd);
    if (platform::key_down('E'))
        Transform::translate(pos, Vec4(0, -1, 0, 0) * spd);
    platform::get_cursor_pos(p1x, p1y);
    float ax = (p1x - p2x) * pi / 360.0f, ay = (p1y - p2y) * pi / 360.0f;
    p2x = p1x;
    p2y = p1y;
    if (a_x + ax > pi * 2)
        a_x += ax - pi * 2;
    else if (a_x + ax < 0)
        a_x += ax + pi * 2;
    else
        a_x += ax;
    if (a_y + ay < pi / 2 && a_y + ay > -pi / 2)
        a_y += ay;
    dir = get_dir();
}
Vec4
Camera::get_dir() const {
    auto d = Vec4(0, 0, 1, 0);
    Transform::rotate(d, Vec4(0, 0, 1, 0), a_y);
    Transform::rotate(d, Vec4(0, 1, 0, 0), a_x);
    d.normalize();
    return d;
}

// ── Graphics (Line/Triangle) implementation ────────────────────
void
Line::draw(Screen& s, const Vec2& a, const Vec2& b, const Vec3& c) {
    bool sh = false;
    int dx = abs(b.x - a.x), dy = abs(b.y - a.y);
    if (dx < dy) {
        int t = dx;
        dx = dy;
        dy = t;
        sh = true;
    }
    int sx = b.x > a.x ? 1 : b.x < a.x ? -1 : 0, sy = b.y > a.y ? 1 : b.y < a.y ? -1 : 0, e = -dx;
    Vec2 p = a;
    for (int i = 0; i < dx; i++) {
        s.set_pixel(p.x, p.y, c, 1);
        if (sh)
            p.y += sy;
        else
            p.x += sx;
        e += 2 * dy;
        if (e >= 0) {
            if (sh)
                p.x += sx;
            else
                p.y += sy;
            e -= 2 * dx;
        }
    }
    s.set_pixel(p.x, p.y, c, 1);
}

void
Line::fast_draw(Screen& s,
                const Vec2& a,
                const Vec2& b,
                const Vec3& color,
                const Material& mat,
                const std::vector<Light>& lights,
                const Vec2* verts,
                const Vec4* v,
                const Vec4& dir,
                const Vec4& normal,
                const Vec3& ambient_col,
                const Tile* scissor) {
    int sx = b.x > a.x ? 1 : b.x < a.x ? -1 : 0;
    if (a.y > s.height - 1 || a.y < 0 || a.x == b.x)
        return;
    auto &A = verts[0], &B = verts[1], &C = verts[2];
    int Dx_a = B.y - C.y, Dy_a = C.x - B.x, Dx_b = C.y - A.y, Dy_b = A.x - C.x, Dx_g = A.y - B.y, Dy_g = B.x - A.x;
    int al = (B - a).cross(C - a), be = (C - a).cross(A - a), ga = (A - a).cross(B - a);
    float i0z = 1 / v[0].z, i1z = 1 / v[1].z, i2z = 1 / v[2].z;
    int sl = abs(a.x - b.x), i0 = 0, i1 = sl;
    if (scissor) {
        if (a.y < scissor->y_start || a.y >= scissor->y_end)
            return;
        int l = a.x, r = b.x;
        if (l > r)
            swap(l, r);
        if (l >= scissor->x_end || r < scissor->x_start)
            return;
        if (sx > 0) {
            if (a.x < scissor->x_start)
                i0 = scissor->x_start - a.x;
            if (b.x >= scissor->x_end)
                i1 = scissor->x_end - 1 - a.x;
        } else {
            if (a.x >= scissor->x_end)
                i0 = a.x - (scissor->x_end - 1);
            if (b.x < scissor->x_start)
                i1 = sl - (scissor->x_start - b.x);
        }
        if (i0 > i1)
            return;
        al += Dx_a * sx * i0;
        be += Dx_b * sx * i0;
        ga += Dx_g * sx * i0;
    }
    int it = i1 - i0;
    float ilc = 1.0f / (float)lights.size();
    if (scissor) {
        int tw = scissor->x_end - scissor->x_start, tox = scissor->x_start, toy = scissor->y_start;
        Vec2 p = { a.x + i0 * sx, a.y };
        for (int i = 0; i <= it; i++) {
            double ad = al, bd = be, gd = ga, sum = ad + bd + gd;
            float id = (float)(1.0 / (ad * i0z + bd * i1z + gd * i2z)), dep = (float)(sum * id);
            size_t idx = (size_t)(p.x - tox) + (size_t)(p.y - toy) * (size_t)tw;
            if (s.z_buffer[idx] == 0 || dep + 2e-7f <= s.z_buffer[idx]) {
                s.z_buffer[idx] = dep;
                Vec4 p3 = (v[0] * (float)(ad * i0z) + v[1] * (float)(bd * i1z) + v[2] * (float)(gd * i2z)) * id;
                auto col = Vec3();
                for (auto& [pos, lcol, it] : lights) {
                    Vec4 L = pos - p3;
                    float Ls = L * L, id2 = fast_rsqrt(Ls), d2 = Ls * id2;
                    float in = it / (0.1f + 0.01f * d2 + 0.1f * Ls);
                    Vec4 Ld = L * id2;
                    float ndl = max(Ld * normal, 0.0f), sp = ndl * ndl * ndl * ndl * ndl;
                    col =
                      col +
                      (color.hadamard(lcol) * ndl * mat.k_diffuse * in / 255 + lcol * in * (sp * mat.k_specular)) * ilc;
                }
                col = col + ambient_col;
                s.buffer[idx] = pack_color(Vec3(min(col.x, 255), min(col.y, 255), min(col.z, 255)));
            }
            p.x += sx;
            if (i < it) {
                al += Dx_a * sx;
                be += Dx_b * sx;
                ga += Dx_g * sx;
            }
        }
    } else {
        Vec2 p = { a.x + i0 * sx, a.y };
        for (int i = 0; i <= it; i++) {
            double ad = al, bd = be, gd = ga, sum = ad + bd + gd;
            float id = (float)(1.0 / (ad * i0z + bd * i1z + gd * i2z)), dep = (float)(sum * id);
            if (s.depth_test(p.x, p.y, dep)) {
                Vec4 p3 = (v[0] * (float)(ad * i0z) + v[1] * (float)(bd * i1z) + v[2] * (float)(gd * i2z)) * id;
                auto col = Vec3();
                for (auto& [pos, lcol, it] : lights) {
                    Vec4 L = pos - p3;
                    float Ls = L * L, id2 = fast_rsqrt(Ls), d2 = Ls * id2;
                    float in = it / (0.1f + 0.01f * d2 + 0.1f * Ls);
                    Vec4 Ld = L * id2;
                    float ndl = max(Ld * normal, 0.0f), sp = ndl * ndl * ndl * ndl * ndl;
                    col =
                      col +
                      (color.hadamard(lcol) * ndl * mat.k_diffuse * in / 255 + lcol * in * (sp * mat.k_specular)) * ilc;
                }
                col = col + ambient_col;
                s.set_pixel(p.x, p.y, Vec3(min(col.x, 255), min(col.y, 255), min(col.z, 255)), dep);
            }
            p.x += sx;
            if (i < it) {
                al += Dx_a * sx;
                be += Dx_b * sx;
                ga += Dx_g * sx;
            }
        }
    }
}

void
Triangle::scan_draw(Screen& s,
                    const Vec2& a,
                    const Vec2& b,
                    const Vec2& c,
                    const Vec3& col,
                    const Material& mat,
                    const std::vector<Light>& lights,
                    const Vec4* v,
                    const Vec4& dir,
                    const Tile* scissor) {
    Vec2 vs[3] = { a, b, c };
    if (vs[1].y < vs[2].y)
        swap(vs[1], vs[2]);
    if (vs[0].y < vs[1].y)
        swap(vs[0], vs[1]);
    if (vs[1].y < vs[2].y)
        swap(vs[1], vs[2]);
    Vec4 n = (v[2] - v[0]).cross3(v[1] - v[0]);
    n.normalize();
    if (vs[0].y == vs[2].y)
        return;
    // Pre-compute ambient contribution (triangle-constant, independent of pixel position)
    Vec3 ambient_col(0, 0, 0);
    {
        float ilc = 1.0f / (float)lights.size(), ka = mat.k_ambient * ilc / 255.0f;
        for (auto& [pos, lcol, it] : lights) {
            ambient_col.x += (int)(col.x * lcol.x * ka);
            ambient_col.y += (int)(col.y * lcol.y * ka);
            ambient_col.z += (int)(col.z * lcol.z * ka);
        }
    }
    int x = (int)((vs[1].y - vs[2].y) * (vs[0].x - vs[2].x) / (double)(vs[0].y - vs[2].y) + .5 + vs[2].x);
    Vec2 vt[3] = { a, b, c };
    fast_draw(s, Vec2(x, vs[1].y), vs[1], col, mat, lights, vt, v, dir, n, ambient_col, scissor);
    float dy = vs[0].y - vs[1].y;
    for (int i = 0; i <= dy; i++)
        fast_draw(s,
                  Vec2((vs[0].x * i + x * (dy - i)) / dy, vs[1].y + i),
                  Vec2((vs[0].x * i + vs[1].x * (dy - i)) / dy, vs[1].y + i),
                  col,
                  mat,
                  lights,
                  vt,
                  v,
                  dir,
                  n,
                  ambient_col,
                  scissor);
    dy = vs[1].y - vs[2].y;
    for (int i = 1; i <= dy; i++)
        fast_draw(s,
                  Vec2((vs[2].x * i + x * (dy - i)) / dy, vs[1].y - i),
                  Vec2((vs[2].x * i + vs[1].x * (dy - i)) / dy, vs[1].y - i),
                  col,
                  mat,
                  lights,
                  vt,
                  v,
                  dir,
                  n,
                  ambient_col,
                  scissor);
}

void
Triangle::scan_draw(Screen& s, const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& col) {
    Vec2 vs[3] = { a, b, c };
    if (vs[1].y < vs[2].y)
        swap(vs[1], vs[2]);
    if (vs[0].y < vs[1].y)
        swap(vs[0], vs[1]);
    if (vs[1].y < vs[2].y)
        swap(vs[1], vs[2]);
    if (vs[0].y == vs[2].y)
        return;
    int x = (int)((vs[1].y - vs[2].y) * (vs[0].x - vs[2].x) / (double)(vs[0].y - vs[2].y) + .5 + vs[2].x);
    draw(s, Vec2(x, vs[1].y), vs[1], col);
    float dy = vs[0].y - vs[1].y;
    for (int i = 0; i <= dy; i++)
        draw(s,
             Vec2((vs[0].x * i + x * (dy - i)) / dy, vs[1].y + i),
             Vec2((vs[0].x * i + vs[1].x * (dy - i)) / dy, vs[1].y + i),
             col);
    dy = vs[1].y - vs[2].y;
    for (int i = 1; i <= dy; i++)
        draw(s,
             Vec2((vs[2].x * i + x * (dy - i)) / dy, vs[1].y - i),
             Vec2((vs[2].x * i + vs[1].x * (dy - i)) / dy, vs[1].y - i),
             col);
}

// ── Transform implementation ───────────────────────────────────
void
Transform::translate(Mesh& m, const Vec4& t) {
    for (auto& v : m.vertices)
        v = t + v;
    m.center = t + m.center;
}
void
Transform::translate(Vec4& p, const Vec4& t) {
    p = p + t;
}
void
Transform::rotate(Mesh& m, const Vec4& c, Vec4 axis, float a) {
    axis.normalize();
    translate(m, c * -1);
    for (auto& i : m.vertices) {
        i = i * cos(a) + axis.cross3(i) * sin(a) + axis * (axis * i) * (1 - cos(a));
        i.w = 1;
    }
    translate(m, c);
}
void
Transform::rotate(Mesh& m, Vec4 axis, float a) {
    axis.normalize();
    for (auto& i : m.vertices) {
        i = i * cos(a) + axis.cross3(i) * sin(a) + axis * (axis * i) * (1 - cos(a));
        i.w = 1;
    }
}
void
Transform::rotate(Vec4& d, Vec4 axis, float a) {
    axis.normalize();
    d = d * cos(a) + axis.cross3(d) * sin(a) + axis * (axis * d) * (1 - cos(a));
    d.w = 0;
}
void
Transform::rotate(Vec3& p, Vec4 axis, float a) {
    axis.normalize();
    auto t = Vec4(p.x, p.y, p.z, 1);
    p = (t * cos(a) + axis.cross3(t) * sin(a) + axis * (axis * t) * (1 - cos(a))).to_vec3();
}
void
Transform::scale(Mesh& mesh, float s) {
    mesh.center.x *= s;
    mesh.center.y *= s;
    mesh.center.z *= s;
    for (auto& v : mesh.vertices) {
        v.x *= s;
        v.y *= s;
        v.z *= s;
    }
    mesh.bounding_radius *= s;
}

// ── Layer2D implementation ─────────────────────────────────────
Layer2D::Layer2D(int w, int h, int z)
  : m_width(w)
  , m_height(h)
  , m_z(z)
  , m_cells((size_t)w * h) {
    clear();
}
void
Layer2D::clear(Vec3 bg) {
    Cell e;
    e.bg = bg;
    e.fg = Vec3(255, 255, 255);
    e.ch = ' ';
    e.transparent = true;
    e.has_text = false;
    std::fill(m_cells.begin(), m_cells.end(), e);
}
void
Layer2D::set_cell(int x, int y, char ch, Vec3 fg, Vec3 bg) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height)
        return;
    auto& c = m_cells[x + y * m_width];
    c.bg = bg;
    c.fg = fg;
    c.ch = ch;
    c.transparent = false;
    c.has_text = (ch != ' ');
}
static int
n2x(float n, int dim) {
    return (int)(n * dim);
}
void
Layer2D::fill_rect(float nx, float ny, float nw, float nh, Vec3 bg) {
    int x = n2x(nx, m_width), y = n2x(ny, m_height), w = n2x(nw, m_width), h = n2x(nh, m_height);
    int x0 = max(0, x), y0 = max(0, y), x1 = min(m_width, x + w), y1 = min(m_height, y + h);
    for (int r = y0; r < y1; r++)
        for (int c = x0; c < x1; c++)
            m_cells[c + r * m_width].fg = bg, m_cells[c + r * m_width].bg = bg,
                            m_cells[c + r * m_width].transparent = false;
}
void
Layer2D::draw_border(float nx, float ny, float nw, float nh, Vec3 col) {
    int x = n2x(nx, m_width), y = n2x(ny, m_height), w = n2x(nw, m_width), h = n2x(nh, m_height);
    for (int i = 1; i < w - 1; i++) {
        set_cell(x + i, y, '\xC4', col, col);
        set_cell(x + i, y + h - 1, '\xC4', col, col);
    }
    for (int i = 1; i < h - 1; i++) {
        set_cell(x, y + i, '\xB3', col, col);
        set_cell(x + w - 1, y + i, '\xB3', col, col);
    }
    set_cell(x, y, '\xDA', col, col);
    set_cell(x + w - 1, y, '\xBF', col, col);
    set_cell(x, y + h - 1, '\xC0', col, col);
    set_cell(x + w - 1, y + h - 1, '\xD9', col, col);
}

// ── Runtime-loaded 5×5 font (from TTF via GDI) ────────────────
void
Layer2D::draw_text(float nx, float ny, const std::string& text, Vec3 fg, Vec3 bg, float scale) {
    int ox = n2x(nx, m_width), oy = n2x(ny, m_height), ch = max(1, (int)(m_height * scale));
    int fh = 5; const uint8_t* font = s_font3x5;
    float is = (float)fh / ch;
    int nc = ch;
    for (size_t ci = 0; ci < text.size(); ci++) {
        unsigned char c = (unsigned char)text[ci];
        if (c < 32 || c > 126)
            c = '?';
        int cx = ox + (int)(ci * ch);
        for (int oy2 = 0; oy2 < nc; oy2++)
            for (int ox2 = 0; ox2 < nc; ox2++) {
                float f0x = ox2 * is, f0y = oy2 * is, f1x = (ox2 + 1) * is, f1y = (oy2 + 1) * is;
                if (f0x >= fh || f0y >= fh)
                    continue;
                int fi = (int)f0x, fj = (int)f0y;
                if (fi < 0) fi = 0; if (fj < 0) fj = 0;
                if (fi < fh && fj < fh && (font[(c - 32) * fh + fj] & (0x80 >> fi)))
                    set_cell(cx + ox2, oy + oy2, ' ', fg, bg);
            }
    }
}
void
Layer2D::draw_line(float nx0, float ny0, float nx1, float ny1, Vec3 col) {
    int x0 = n2x(nx0, m_width), y0 = n2x(ny0, m_height), x1 = n2x(nx1, m_width), y1 = n2x(ny1, m_height);
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
    while (true) {
        set_cell(x0, y0, ' ', col, col);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}
void
Layer2D::composite_to(std::vector<Color>& t, int tw, int th) const {
    if (!m_visible)
        return;
    int w = min(m_width, tw), h = min(m_height, th);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            auto& c = m_cells[x + y * m_width];
            if (!c.transparent)
                t[x + y * tw] = pack_color(c.fg);
        }
}

// ── TUI implementation ─────────────────────────────────────────
Label::Label(float nx, float ny, const std::string& text, Vec3 fg, float fs)
  : Widget(nx, ny, text.size() * 8 * fs / 960, 8 * fs / 330)
  , m_text(text)
  , m_fg(fg)
  , m_scale(fs) {}
void
Label::draw(Layer2D& l) {
    l.draw_text(m_nx, m_ny, m_text, m_fg, Vec3(0, 0, 0), m_scale);
}
Button::Button(float nx, float ny, const std::string& text, std::function<void()> cb, Vec3 bg, Vec3 fg, float fs)
  : Widget(nx, ny, (text.size() * 8 * fs + 4) / 960, (8 * fs + 4) / 330)
  , m_text(text)
  , m_cb(cb)
  , m_bg(bg)
  , m_fg(fg)
  , m_scale(fs) {}
void
Button::draw(Layer2D& l) {
    l.fill_rect(m_nx, m_ny, m_nw, m_nh, m_bg);
    l.draw_border(m_nx, m_ny, m_nw, m_nh, m_fg);
    l.draw_text(m_nx + .002f, m_ny + .005f, m_text, m_fg, m_bg, m_scale);
}
bool
Button::handle_click(float nmx, float nmy) {
    return (nmx >= m_nx && nmx < m_nx + m_nw && nmy >= m_ny && nmy < m_ny + m_nh) ? (m_cb ? m_cb(), true : true)
                                                                                  : false;
}

// ── Renderer implementation ────────────────────────────────────

static int
hw_threads() {
    return max(1, (int)thread::hardware_concurrency());
}

Renderer::Renderer() {
    Screen::init();
    mesh_num = 0;
    light_num = 0;
}
Renderer::~Renderer() {
    shutdown();
}

void
Renderer::update() {
    screen.update();
    camera.update_from(screen);
    render_frame();
}

void
Renderer::render_frame() {
    Stopwatch sw;
    if ((size_t)screen.width * screen.height != screen.buffer.size()) {
        screen.buffer.assign(screen.width * screen.height, 0);
        screen.z_buffer.assign(screen.width * screen.height, 0);
    }
    prepare_frame();
    frustum_cull();
    camera.pre_project_verts(frame_meshes, frame_vert_proj);
    // Fixed grid tile partition (avoids reallocation spikes from adaptive partitioning)
    tiles = partition_tiles(screen.width, screen.height, max(num_threads * 16, 128));
    if (aa_mode == TAA) {
        Vec2 j = Screen::halton_sequence(screen.get_taa_frame_count() + 1);
        camera.set_jitter(j.x / 1e6f, j.y / 1e6f);
    }     else
        camera.set_jitter(0, 0);
    tile_index.store(0, std::memory_order_relaxed);
    workers_done.store(0, std::memory_order_release);
    current_frame.fetch_add(1, std::memory_order_acq_rel);
    cv.notify_all();
    {
        TileScreen ts(0, 0, 0, 0, 0, 0);
        int ti;
        while ((ti = tile_index.fetch_add(1, std::memory_order_relaxed)) < (int)tiles.size())
            render_tile(tiles[ti], ts);
    }
    while (workers_done.load(std::memory_order_acquire) < num_threads)
        std::this_thread::yield();

    // Build Hi-Z from screen.z_buffer (now populated by all tiles)
    {
        const int hw = max(1, screen.width / 8);
        const int hh = max(1, screen.height / 8);
        hiz_buffer.assign((size_t)hw * hh, 1e9f);
        hiz_w = hw;
        hiz_h = hh;
        for (int y = 0; y < hh; y++)
            for (int x = 0; x < hw; x++) {
                float d = 1e9f;
                for (int dy = 0; dy < 8; dy++)
                    for (int dx = 0; dx < 8; dx++) {
                        int sx = x * 8 + dx, sy = y * 8 + dy;
                        if (sx < screen.width && sy < screen.height) {
                            float zd = screen.z_buffer[sx + sy * screen.width];
                            if (zd > 0.001f && zd < d)
                                d = zd;
                        }
                    }
                if (d < 1e8f)
                    hiz_buffer[x + y * hw] = d;
            }
    }

    composite_layers();
    composite_frame();
    double fm = sw.elapsed_ms();
    screen.calculate_fps(fm);
}

void
Renderer::frustum_cull() {
    size_t n = frame_meshes.size();
    mesh_visible.assign(n, true);
    if (!n)
        return;
    float ca = cos(camera.a_x), sa = sin(camera.a_x), cb = cos(camera.a_y), sb = sin(camera.a_y);
    Mat4 Ry(Vec4(ca, 0, -sa, 0), Vec4(0, 1, 0, 0), Vec4(sa, 0, ca, 0), Vec4(0, 0, 0, 1));
    Mat4 Rx(Vec4(1, 0, 0, 0), Vec4(0, cb, sb, 0), Vec4(0, -sb, cb, 0), Vec4(0, 0, 0, 1));
    Mat4 T(Vec4(1, 0, 0, -camera.pos.x), Vec4(0, 1, 0, -camera.pos.y), Vec4(0, 0, 1, -camera.pos.z), Vec4(0, 0, 0, 1));
    Mat4 vm = Rx * Ry * T;
    float cn = 1, cf = 100, asp = camera.width, i2n = 1 / (2 * cn);
    for (size_t i = 0; i < n; i++) {
        auto& m = frame_meshes[i];
        Vec4 cc = vm * m.center;
        float cx = cc.x, cy = cc.y, cz = cc.z, r = m.bounding_radius;
        if (r <= 0)
            continue;
        if (cz + r < cn || cz - r > cf) {
            mesh_visible[i] = false;
            continue;
        }
        float zt = max(cz + r, cn), hw = asp * zt * i2n;
        if (cx - r > hw || cx + r < -hw) {
            //mesh_visible[i] = false;
            continue;
        }
        float hh = zt * i2n;
        if (cy - r > hh || cy + r < -hh) {
            //mesh_visible[i] = false;
            continue;
        }
        if (hiz_test(cx, cy, cz, r))
            mesh_visible[i] = false;
    }
}

void
Renderer::prepare_frame() {
    if (frame_meshes.size() != meshes.size())
        frame_meshes = meshes;
    else
        for (size_t i = 0; i < meshes.size(); i++)
            frame_meshes[i] = meshes[i];
    float ca = cos(camera.a_x), sa = sin(camera.a_x), cb = cos(camera.a_y), sb = sin(camera.a_y);
    Mat4 Ry(Vec4(ca, 0, -sa, 0), Vec4(0, 1, 0, 0), Vec4(sa, 0, ca, 0), Vec4(0, 0, 0, 1));
    Mat4 Rx(Vec4(1, 0, 0, 0), Vec4(0, cb, sb, 0), Vec4(0, -sb, cb, 0), Vec4(0, 0, 0, 1));
    Mat4 T(Vec4(1, 0, 0, -camera.pos.x), Vec4(0, 1, 0, -camera.pos.y), Vec4(0, 0, 1, -camera.pos.z), Vec4(0, 0, 0, 1));
    Mat4 vm = Rx * Ry * T;
    for (auto& m : frame_meshes)
        for (auto& v : m.vertices)
            v = vm * v;
    frame_lights = lights;
    for (auto& l : frame_lights)
        l.pos = vm * l.pos;
}

void
Renderer::composite_layers() {
    if (overlay.width() != screen.width || overlay.height() != screen.height)
        overlay = Layer2D(screen.width, screen.height, 10);
    overlay.clear();
    if (screen.show_fps)
        overlay.draw_text(.01f, .01f, Screen::get_fps_display(), Vec3(200, 200, 200), Vec3(0, 0, 0), .025f);
    overlay.composite_to(screen.buffer, screen.width, screen.height);
}

void
Renderer::composite_frame() {
    switch (aa_mode) {
        case FXAA:
            screen.apply_fxaa();
            break;
        case TAA:
            screen.apply_taa();
            break;
        default:
            break;
    }
    screen.draw();
    screen.show();
}

void
Renderer::render_tile(const Tile& tile, TileScreen& ts) {
    int tw = tile.x_end - tile.x_start, th = tile.y_end - tile.y_start;
    if (screen.SSAA) {
        int tw2 = tw * 2, th2 = th * 2;
        ts.resize_if_needed(screen.width * 2, screen.height * 2, tile.x_start * 2, tile.y_start * 2, tw2, th2);
        Tile c2{ tile.x_start * 2, tile.x_end * 2, tile.y_start * 2, tile.y_end * 2 };
        const Vec4* pp = frame_vert_proj.data();
        for (int mi = 0; mi < (int)frame_meshes.size(); mi++) {
            if (mi < (int)mesh_visible.size() && !mesh_visible[mi]) {
                pp += frame_meshes[mi].vertices.size();
                continue;
            }
            camera.load(ts, frame_meshes[mi], frame_lights, &c2, pp);
            pp += frame_meshes[mi].vertices.size();
        }
        size_t r1 = (size_t)screen.width;
        // 1) Downsample color → ts.buffer (local, cache-hot write)
        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++) {
                size_t s = (size_t)(x * 2) + (size_t)(y * 2) * (size_t)tw2;
                Vec3 a0 = unpack_color(ts.buffer[s]), a1 = unpack_color(ts.buffer[s + 1]),
                     a2 = unpack_color(ts.buffer[s + tw2]), a3 = unpack_color(ts.buffer[s + tw2 + 1]);
                ts.buffer[(size_t)x + (size_t)y * (size_t)tw] = pack_color(
                  Vec3((a0.x + a1.x + a2.x + a3.x) / 4, (a0.y + a1.y + a2.y + a3.y) / 4,
                       (a0.z + a1.z + a2.z + a3.z) / 4));
            }
        // 2) Copy downsampled color → screen.buffer (fast memcpy)
        for (int y = 0; y < th; y++)
            memcpy(&screen.buffer[(size_t)tile.x_start + (size_t)(tile.y_start + y) * r1],
                   &ts.buffer[(size_t)y * (size_t)tw],
                   (size_t)tw * sizeof(Color));
        // 3) Downsample z: min of 2×2 block → screen.z_buffer
        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++) {
                size_t s = (size_t)(x * 2) + (size_t)(y * 2) * (size_t)tw2;
                float zd = ts.z_buffer[s];
                if (ts.z_buffer[s + 1] > 0.001f && ts.z_buffer[s + 1] < zd) zd = ts.z_buffer[s + 1];
                if (ts.z_buffer[s + tw2] > 0.001f && ts.z_buffer[s + tw2] < zd) zd = ts.z_buffer[s + tw2];
                if (ts.z_buffer[s + tw2 + 1] > 0.001f && ts.z_buffer[s + tw2 + 1] < zd) zd = ts.z_buffer[s + tw2 + 1];
                screen.z_buffer[(size_t)tile.x_start + x + (size_t)(tile.y_start + y) * r1] = zd;
            }
    } else {
        ts.resize_if_needed(screen.width, screen.height, tile.x_start, tile.y_start, tw, th);
        const Vec4* pp = frame_vert_proj.data();
        for (int mi = 0; mi < (int)frame_meshes.size(); mi++) {
            if (mi < (int)mesh_visible.size() && !mesh_visible[mi]) {
                pp += frame_meshes[mi].vertices.size();
                continue;
            }
            camera.load(ts, frame_meshes[mi], frame_lights, &tile, pp);
            pp += frame_meshes[mi].vertices.size();
        }
        size_t rw = (size_t)screen.width;
        for (int y = 0; y < th; y++) {
            memcpy(&screen.buffer[(size_t)tile.x_start + (size_t)(tile.y_start + y) * rw],
                   &ts.buffer[(size_t)y * (size_t)tw],
                   (size_t)tw * sizeof(Color));
            memcpy(&screen.z_buffer[(size_t)tile.x_start + (size_t)(tile.y_start + y) * rw],
                   &ts.z_buffer[(size_t)y * (size_t)tw],
                   (size_t)tw * sizeof(float));
        }
    }
}

void
Renderer::worker_loop(int id) {
    int mf = 0;
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !running || current_frame.load(std::memory_order_acquire) != mf; });
        if (!running)
            return;
        lock.unlock();
        TileScreen ts(0, 0, 0, 0, 0, 0);
        int ti;
        while ((ti = tile_index.fetch_add(1, std::memory_order_relaxed)) < (int)tiles.size())
            render_tile(tiles[ti], ts);
        mf = current_frame.load(std::memory_order_acquire);
        workers_done.fetch_add(1, std::memory_order_release);
    }
}

void
Renderer::toggle_fps() {
    screen.show_fps = !screen.show_fps;
}
void
Renderer::set_aa(AA aa) {
    aa_mode = aa;
    screen.SSAA = (aa == SSAA);
}
void
Renderer::set_camera_pos(const Vec4& p) {
    camera.pos = p;
}
void
Renderer::launch() {
    if (running)
        return;
    num_threads = hw_threads();
    running = true;
    for (int i = 0; i < num_threads; i++)
        workers.emplace_back(&Renderer::worker_loop, this, i);
}
void
Renderer::shutdown() {
    if (!running)
        return;
    running = false;
    cv.notify_all();
    for (auto& w : workers)
        if (w.joinable())
            w.join();
    workers.clear();
}

ID
Renderer::add_meshes(const std::vector<Mesh>& ms) {
    int b = mesh_num;
    for (auto& m : ms) {
        meshes.push_back(m);
        ++mesh_num;
    }
    return { b, mesh_num };
}
void
Renderer::remove_meshes(ID id) {
    if (id.begin < 0 || id.end > (int)meshes.size() || id.begin >= id.end)
        return;
    meshes.erase(meshes.begin() + id.begin, meshes.begin() + id.end);
    mesh_num = (int)meshes.size();
}
void
Renderer::clear_meshes() {
    meshes.clear();
    mesh_num = 0;
}
std::vector<Mesh*>
Renderer::get_meshes(ID id) {
    std::vector<Mesh*> r;
    for (int i = id.begin; i < id.end && i < (int)meshes.size(); i++)
        r.push_back(&meshes[i]);
    return r;
}

ID
Renderer::add_lights(const std::vector<Light>& ls) {
    int b = light_num;
    for (auto& l : ls) {
        lights.push_back(l);
        ++light_num;
    }
    return { b, light_num };
}
void
Renderer::remove_lights(ID id) {
    if (id.begin < 0 || id.end > (int)lights.size() || id.begin >= id.end)
        return;
    lights.erase(lights.begin() + id.begin, lights.begin() + id.end);
    light_num = (int)lights.size();
}
void
Renderer::clear_lights() {
    lights.clear();
    light_num = 0;
}
std::vector<Light*>
Renderer::get_lights(ID id) {
    std::vector<Light*> r;
    for (int i = id.begin; i < id.end && i < (int)lights.size(); i++)
        r.push_back(&lights[i]);
    return r;
}

void
Renderer::controller() {
    camera.controller();
}
void
Renderer::hiz_clear() {
    hiz_w = max(1, screen.width / 8);
    hiz_h = max(1, screen.height / 8);
    hiz_buffer.assign((size_t)hiz_w * hiz_h, 1e9f);
}
bool
Renderer::hiz_test(float cx, float cy, float cz, float r) const {
    if (cz < 1 || hiz_w == 0 || hiz_h == 0)
        return false;
    // Project bounding sphere center + radius to screen space
    float sx = cx * screen.height / cz + screen.width * .5f,
          sy = -cy * screen.height / cz + screen.height * .5f,
          sr = max(1.0f, r * screen.height / cz);
    int hx0 = max(0, (int)((sx - sr) / 8));
    int hy0 = max(0, (int)((sy - sr) / 8));
    int hx1 = min(hiz_w - 1, (int)((sx + sr) / 8));
    int hy1 = min(hiz_h - 1, (int)((sy + sr) / 8));
    // Only cull if ALL cells covered by the sphere have closer geometry
    for (int hy = hy0; hy <= hy1; hy++)
        for (int hx = hx0; hx <= hx1; hx++) {
            if (cz - r <= hiz_buffer[hx + hy * hiz_w] + 1.0f)
                return false;
        }
    return true;
}
void
Renderer::hiz_update(int x, int y, int w, int h, const std::vector<float>& zb, int zw) {
    for (int ty = y; ty < y + h; ty++)
        for (int tx = x; tx < x + w; tx++) {
            int hx = tx / 8, hy = ty / 8;
            if (hx >= 0 && hx < hiz_w && hy >= 0 && hy < hiz_h) {
                float zd = zb[(size_t)(tx - x) + (size_t)(ty - y) * (size_t)w];
                if (zd > .001f) {
                    float& hz = hiz_buffer[hx + hy * hiz_w];
                    if (zd < hz)
                        hz = zd;
                }
            }
        }
}

// ── RubikCube implementation (unqualified types fixed below) ───────────────────────────────────
RubikCube::RubikCube(float l) {
    float len = l / 3;
    for (int i = 0; i < 27; ++i) {
        meshes.push_back(gen_block(Vec4(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1, 1), len));
        block_indices[i] = Vec3(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1);
    }
}
Mesh
RubikCube::gen_block(const Vec4& p, float l) {
    constexpr Material mt{ 0.4f, 0.8f, 0.3f };
    Vec3 col[6] = { Vec3(255, 0, 0), Vec3(0, 255, 0),   Vec3(255, 88, 0),
                    Vec3(0, 0, 255), Vec3(255, 255, 0), Vec3(255, 255, 255) };
    Vec4 ctr = p * l * 1.04f;
    float r = l / 2;
    std::vector<Vec4> verts = { { ctr.x - r, ctr.y - r, ctr.z - r, 1 }, { ctr.x + r, ctr.y - r, ctr.z - r, 1 },
                                { ctr.x + r, ctr.y + r, ctr.z - r, 1 }, { ctr.x - r, ctr.y + r, ctr.z - r, 1 },
                                { ctr.x - r, ctr.y - r, ctr.z + r, 1 }, { ctr.x + r, ctr.y - r, ctr.z + r, 1 },
                                { ctr.x + r, ctr.y + r, ctr.z + r, 1 }, { ctr.x - r, ctr.y + r, ctr.z + r, 1 } };
    static const int fi[6][6] = { { 0, 1, 2, 2, 3, 0 }, { 1, 5, 6, 6, 2, 1 }, { 5, 4, 7, 7, 6, 5 },
                                  { 4, 0, 3, 3, 7, 4 }, { 3, 2, 6, 6, 7, 3 }, { 1, 0, 4, 4, 5, 1 } };
    Vec3 fc[6] = { p.z == -1 ? col[0] : Vec3(200, 200, 200), p.x == 1 ? col[1] : Vec3(200, 200, 200),
                   p.z == 1 ? col[2] : Vec3(200, 200, 200),  p.x == -1 ? col[3] : Vec3(200, 200, 200),
                   p.y == 1 ? col[4] : Vec3(200, 200, 200),  p.y == -1 ? col[5] : Vec3(200, 200, 200) };
    std::vector<int> idx;
    std::vector<Vec3> mc;
    for (int f = 0; f < 6; f++)
        for (int j = 0; j < 6; j++) {
            idx.push_back(fi[f][j]);
            mc.push_back(fc[f]);
        }
    return { ctr, verts, idx, mc, mt };
}
void
RubikCube::control(Vec3* blk, const std::vector<Mesh*>& meshes) {
    static Vec4 ab[3] = { Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0) };
    static bool anim = false;
    static int af;
    static std::vector<int> ablk;
    static int afr = 0;
    static constexpr int TF = 9;
    if (!anim && platform::kbhit()) {
        Vec4 wa;
        std::vector<int> bk;
        int face = -1;
        bool rot = false;
        switch (platform::getch()) {
            case 'f':
                wa = Vec4(0, 0, -1, 0);
                face = 0;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].z == -1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            case 'b':
                wa = Vec4(0, 0, 1, 0);
                face = 1;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].z == 1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            case 'l':
                wa = Vec4(-1, 0, 0, 0);
                face = 2;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].x == -1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            case 'r':
                wa = Vec4(1, 0, 0, 0);
                face = 3;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].x == 1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            case 'd':
                wa = Vec4(0, -1, 0, 0);
                face = 4;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].y == -1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            case 'u':
                wa = Vec4(0, 1, 0, 0);
                face = 5;
                for (int i = 0; i < 27; ++i)
                    if (blk[i].y == 1) {
                        bk.push_back(i);
                        rot = true;
                    }
                break;
            default:
                break;
        }
        if (rot) {
            for (int i : bk)
                Transform::rotate(blk[i], wa, pi / 2);
            anim = true;
            af = face;
            ablk = std::move(bk);
            afr = TF;
        }
    }
    if (anim) {
        static const Vec4 fa[6] = { Vec4(0, 0, -1, 0), Vec4(0, 0, 1, 0),  Vec4(-1, 0, 0, 0),
                                    Vec4(1, 0, 0, 0),  Vec4(0, -1, 0, 0), Vec4(0, 1, 0, 0) };
        const Vec4& b = fa[af];
        Vec4 sa(b.x * ab[0].x + b.y * ab[1].x + b.z * ab[2].x,
                b.x * ab[0].y + b.y * ab[1].y + b.z * ab[2].y,
                b.x * ab[0].z + b.y * ab[1].z + b.z * ab[2].z,
                0);
        for (int i : ablk)
            Transform::rotate(*meshes[i], sa, pi / 18);
        if (--afr <= 0)
            anim = false;
    }
    static int p1x = 0, p1y = 0, p2x = 0, p2y = 0;
    platform::get_cursor_pos(p1x, p1y);
    float ax = (p2x - p1x) * pi / 360, ay = (p2y - p1y) * pi / 360;
    p2x = p1x;
    p2y = p1y;
    if (platform::key_down(VK_RBUTTON)) {
        for (auto& a : ab) {
            Transform::rotate(a, Vec4(0, 1, 0, 0), ax);
            Transform::rotate(a, Vec4(1, 0, 0, 0), ay);
        }
        for (auto& m : meshes) {
            Transform::rotate(*m, Vec4(0, 1, 0, 0), ax);
            Transform::rotate(*m, Vec4(1, 0, 0, 0), ay);
        }
    }
}

#endif // RENDER_ENGINE_IMPLEMENTATION
