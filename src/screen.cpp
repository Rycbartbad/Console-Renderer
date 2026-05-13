#include "screen.h"
#include <iostream>
#include <windows.h>

clock_t Screen::start_t = 0;
clock_t Screen::end_t = 0;
int Screen::counter_t = 0;
int Screen::fps = 0;

Screen::Screen() {
    width = height = 0;
    fps = 0;
    bias = false;
    SSAA = false;
    show_fps = false;
}

void Screen::init() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    printf("\033[?25l");
    start_t = clock();
    counter_t = 0;
}

void Screen::update() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    csbi.dwSize.X = csbi.dwMaximumWindowSize.X;
    csbi.dwSize.Y = csbi.dwMaximumWindowSize.Y;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);
    width = csbi.dwSize.X / 2;
    height = csbi.dwSize.Y;
    bias = csbi.dwSize.X % 2;
    // SSAA doubling is now handled per-worker (render at 2x, downscale to 1x before tile copy)
    // Screen stays at display resolution.
}

void Screen::set_pixel(const int& x, const int& y, const Vec3& color, const float& z) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    const size_t index = x + y * width;

    if (z_buffer[index] != 0 && z >= z_buffer[index]) return;
    z_buffer[index] = z;

    buffer[index] = color;
}

bool Screen::depth_test(const int x, const int y, const float z) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    const size_t index = static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(width);
    return z_buffer[index] == 0 || z < z_buffer[index];
}

void Screen::clear() {
    buffer.assign(width * height, Vec3());
    z_buffer.assign(width * height, 0);
}

void Screen::draw() {
    const int w = width;
    const int h = height;
    const size_t pixel_count = static_cast<size_t>(w) * static_cast<size_t>(h);

    // Pre-allocate output buffer to avoid repeated reallocation from +=
    output_buf.reserve(static_cast<size_t>(w) * h * 30 + 1024);

    // First frame or resize: full render with screen clear
    if (prev_buffer.size() != pixel_count) {
        prev_buffer.assign(pixel_count, Vec3());
        output_buf = "\033[3J\033[H";
        if (show_fps)
            output_buf = "\033[3J\033[H\033[mFPS:" + std::to_string(fps) + " ";
        for (int y = 0; y < h; y++) {
            int x = 0;
            while (x < w) {
                const auto& c = buffer[x + y * w];
                int r = std::min(c.x, 255), g = std::min(c.y, 255), b = std::min(c.z, 255);
                int run = 1;
                while (x + run < w) {
                    const auto& n = buffer[x + run + y * w];
                    if (std::min(n.x, 255) != r || std::min(n.y, 255) != g || std::min(n.z, 255) != b) break;
                    run++;
                }
                output_buf += "\033[48;2;" + std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + "m" +
                    std::string(static_cast<size_t>(run * 2), ' ');
                x += run;
            }
            if (bias) output_buf += "\033[m ";
        }
        prev_buffer = buffer;
        return;
    }

    // ── Diff-based output: only emit ANSI codes for changed pixels ──
    output_buf = "\033[H";
    if (show_fps)
        output_buf = "\033[H\033[mFPS:" + std::to_string(fps) + " ";

    for (int y = 0; y < h; y++) {
        bool row_dirty = false;
        int x = 0;
        while (x < w) {
            const size_t idx = static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(w);
            const Vec3& cur = buffer[idx];
            const Vec3& prev = prev_buffer[idx];
            if (cur.x == prev.x && cur.y == prev.y && cur.z == prev.z) {
                x++;
                continue;
            }
            if (!row_dirty) {
                row_dirty = true;
                output_buf += "\033[" + std::to_string(y + 1) + ";1H";
            }
            // Run-length: consecutive pixels with the same color (changed or not)
            int run = 1;
            while (x + run < w) {
                const Vec3& nxt = buffer[static_cast<size_t>(x + run) + static_cast<size_t>(y) * static_cast<size_t>(w)];
                if (nxt.x != cur.x || nxt.y != cur.y || nxt.z != cur.z) break;
                run++;
            }
            int r = std::min(cur.x, 255), g = std::min(cur.y, 255), b = std::min(cur.z, 255);
            output_buf += "\033[" + std::to_string(x * 2 + 1) + "G\033[48;2;" +
                std::to_string(r) + ';' + std::to_string(g) + ';' + std::to_string(b) + "m" +
                std::string(static_cast<size_t>(run * 2), ' ');
            x += run;
        }
    }

    prev_buffer = buffer;
}

void Screen::calculate_fps() {
    ++counter_t;
    end_t = clock();
    if (end_t - start_t == 0) return;
    if (counter_t >= 100) {
        
        fps = CLOCKS_PER_SEC * 100 / (end_t - start_t);
        start_t = clock();
        counter_t = 0;
    }
}

void Screen::apply_ssaa() {
    // Downsample 2×2 blocks to 1 pixel — inlined for speed, no lambda/ptr-to-member overhead
    if (width < 2 || height < 2) return;
    const int w2 = width, h2 = height;
    const int nw = w2 / 2, nh = h2 / 2;
    aa_scratch.assign(nw * nh, Vec3());
    for (int y = 0; y < nh; y++) {
        const size_t row_base = static_cast<size_t>(y * 2) * static_cast<size_t>(w2);
        size_t didx = static_cast<size_t>(y) * static_cast<size_t>(nw);
        for (int x = 0; x < nw; x++) {
            const size_t s = static_cast<size_t>(x * 2) + row_base;
            const Vec3& a = buffer[s];
            const Vec3& b = buffer[s + 1];
            const Vec3& c = buffer[s + w2];
            const Vec3& d = buffer[s + w2 + 1];
            aa_scratch[didx + x] = Vec3(
                (a.x + b.x + c.x + d.x) / 4,
                (a.y + b.y + c.y + d.y) / 4,
                (a.z + b.z + c.z + d.z) / 4);
        }
    }
    buffer.swap(aa_scratch);
    z_buffer.assign(nw * nh, 0);
    width = nw;
    height = nh;
}

void Screen::apply_fxaa() {
    if (buffer.empty()) return;

    aa_scratch = buffer;

    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            const size_t idx = x + y * width;
            const Vec3& center = buffer[idx];

            // Luminance of center and 4 cross neighbors
            const Vec3& left   = buffer[(x - 1) + y * width];
            const Vec3& right  = buffer[(x + 1) + y * width];
            const Vec3& top    = buffer[x + (y - 1) * width];
            const Vec3& bottom = buffer[x + (y + 1) * width];

            auto lum = [](const Vec3& c) {
                return 0.299f * c.x + 0.587f * c.y + 0.114f * c.z;
            };

            const float luma[5] = {
                lum(center), lum(left), lum(right), lum(top), lum(bottom)
            };

            float min_lum = luma[0], max_lum = luma[0];
            for (int i = 1; i < 5; i++) {
                if (luma[i] < min_lum) min_lum = luma[i];
                if (luma[i] > max_lum) max_lum = luma[i];
            }

            const float contrast = max_lum - min_lum;
            if (contrast < 0.1f * max_lum) continue;  // smooth area, skip

            // Edge direction via luminance gradient
            const float horz_grad = std::abs(luma[1] - luma[2]);  // left-right
            const float vert_grad = std::abs(luma[3] - luma[4]);  // top-bottom

            Vec3 result;
            if (horz_grad > vert_grad) {
                // Horizontal edge → blend vertically
                const int ry0 = (std::max)(y - 1, 0);
                const int ry1 = (std::min)(y + 1, height - 1);
                const Vec3& above = buffer[x + ry0 * width];
                const Vec3& below = buffer[x + ry1 * width];
                result = Vec3(
                    static_cast<int>(lerp(static_cast<float>(center.x),
                        (above.x + below.x) * 0.5f, 0.5f)),
                    static_cast<int>(lerp(static_cast<float>(center.y),
                        (above.y + below.y) * 0.5f, 0.5f)),
                    static_cast<int>(lerp(static_cast<float>(center.z),
                        (above.z + below.z) * 0.5f, 0.5f))
                );
            } else {
                // Vertical edge → blend horizontally
                const int rx0 = (std::max)(x - 1, 0);
                const int rx1 = (std::min)(x + 1, width - 1);
                const Vec3& left_neighbor = buffer[rx0 + y * width];
                const Vec3& right_neighbor = buffer[rx1 + y * width];
                result = Vec3(
                    static_cast<int>(lerp(static_cast<float>(center.x),
                        (left_neighbor.x + right_neighbor.x) * 0.5f, 0.5f)),
                    static_cast<int>(lerp(static_cast<float>(center.y),
                        (left_neighbor.y + right_neighbor.y) * 0.5f, 0.5f)),
                    static_cast<int>(lerp(static_cast<float>(center.z),
                        (left_neighbor.z + right_neighbor.z) * 0.5f, 0.5f))
                );
            }

            aa_scratch[idx] = Vec3(
                clamp(result.x, 0, 255),
                clamp(result.y, 0, 255),
                clamp(result.z, 0, 255)
            );
        }
    }

    buffer.swap(aa_scratch);
}

void Screen::apply_taa() {
    if (buffer.empty()) return;

    const size_t pixel_count = width * height;

    // First frame or size mismatch → initialize history
    if (taa_history.size() != pixel_count) {
        taa_history = buffer;
        return;
    }

    aa_scratch.assign(pixel_count, Vec3());

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t idx = x + y * width;
            const Vec3& current = buffer[idx];
            const Vec3& history = taa_history[idx];

            // AABB from 3x3 neighborhood of current frame
            int min_r = 255, max_r = 0;
            int min_g = 255, max_g = 0;
            int min_b = 255, max_b = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    const int nx = x + dx;
                    const int ny = y + dy;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

                    const Vec3& n = buffer[nx + ny * width];
                    if (n.x < min_r) min_r = n.x;
                    if (n.x > max_r) max_r = n.x;
                    if (n.y < min_g) min_g = n.y;
                    if (n.y > max_g) max_g = n.y;
                    if (n.z < min_b) min_b = n.z;
                    if (n.z > max_b) max_b = n.z;
                }
            }

            // Clamp history color to neighborhood AABB (anti-ghosting)
            Vec3 clamped;
            clamped.x = clamp(history.x, min_r, max_r);
            clamped.y = clamp(history.y, min_g, max_g);
            clamped.z = clamp(history.z, min_b, max_b);

            // Blend: 85% history, 15% current
            aa_scratch[idx] = Vec3(
                static_cast<int>(lerp(static_cast<float>(clamped.x),
                    static_cast<float>(current.x), 0.15f)),
                static_cast<int>(lerp(static_cast<float>(clamped.y),
                    static_cast<float>(current.y), 0.15f)),
                static_cast<int>(lerp(static_cast<float>(clamped.z),
                    static_cast<float>(current.z), 0.15f))
            );
        }
    }

    buffer.swap(aa_scratch);
    taa_history = buffer;
    taa_frame_count++;
}

Vec2 Screen::halton_sequence(int index) {
    if (index < 1) index = 1;

    // Halton base 2 for x
    float f = 1.0f;
    float x = 0.0f;
    int i = index;
    while (i > 0) {
        f *= 0.5f;
        x += (i & 1) * f;
        i >>= 1;
    }

    // Halton base 3 for y
    f = 1.0f;
    float y = 0.0f;
    i = index;
    while (i > 0) {
        f /= 3.0f;
        y += static_cast<float>(i % 3) * f;
        i /= 3;
    }

    // Center around zero: range [-0.5, 0.5]
    x -= 0.5f;
    y -= 0.5f;

    // Scale to preserve precision in int Vec2; caller divides by SCALE to recover float
    constexpr int SCALE = 1000000;
    return Vec2(static_cast<int>(x * SCALE), static_cast<int>(y * SCALE));
}

void Screen::show() const {
    std::cout << output_buf;
}