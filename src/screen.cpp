#include "screen.h"
#include "platform.h"
#include <cstdio>

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

void Screen::init() {
    platform::console_init();
}

void Screen::update() {
    platform::console_get_size(width, height, bias);
    // SSAA doubling handled per-worker, Screen stays at display resolution.
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

// Fast integer-to-string append for 0-9999 (covers color values 0-255 and
// cursor positions up to x*2+1 �?1923).
static void append_uint(std::string& buf, int n) {
    if (n >= 1000) {
        buf += static_cast<char>('0' + n / 1000);
        buf += static_cast<char>('0' + (n / 100) % 10);
        buf += static_cast<char>('0' + (n / 10) % 10);
        buf += static_cast<char>('0' + n % 10);
    } else if (n >= 100) {
        buf += static_cast<char>('0' + n / 100);
        buf += static_cast<char>('0' + (n / 10) % 10);
        buf += static_cast<char>('0' + n % 10);
    } else if (n >= 10) {
        buf += static_cast<char>('0' + n / 10);
        buf += static_cast<char>('0' + n % 10);
    } else {
        buf += static_cast<char>('0' + n);
    }
}

// Append spaces using REP CSI n b (repeat last char) when beneficial.
// \x1b[ Nb  repeats the last graphic character N times �?supported by
// Windows Terminal, iTerm2, Kitty, xterm, etc.  Saves bandwidth for runs �?3 pixels.
static void append_spans(std::string& buf, int count) {
    if (count <= 0) return;
    if (count < 6) {  // runs �?3 pixels: plain spaces are shorter than ESC sequence
        buf.append(static_cast<size_t>(count), ' ');
    } else {
        buf += ' ';  // prime the repeat buffer with the space character
        buf += "\x1b[";
        append_uint(buf, count - 1);
        buf += 'b';
    }
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
        if (show_fps) {
            output_buf += "\033[mFPS:";
            append_uint(output_buf, fps);
            output_buf += ' ';
        }
        for (int y = 0; y < h; y++) {
            int x = 0;
            while (x < w) {
                const auto& c = buffer[x + y * w];
                int run = 1;
                while (x + run < w) {
                    const auto& n = buffer[x + run + y * w];
                    if (n.x != c.x || n.y != c.y || n.z != c.z) break;
                    run++;
                }
                output_buf += "\033[48;2;";
                append_uint(output_buf, c.x > 255 ? 255 : c.x < 0 ? 0 : c.x);
                output_buf += ';';
                append_uint(output_buf, c.y > 255 ? 255 : c.y < 0 ? 0 : c.y);
                output_buf += ';';
                append_uint(output_buf, c.z > 255 ? 255 : c.z < 0 ? 0 : c.z);
                output_buf += 'm';
                append_spans(output_buf, run * 2);
                x += run;
            }
            if (bias) output_buf += "\033[m ";
        }
        prev_buffer = buffer;
        return;
    }

    // ── Diff-based output: only emit ANSI codes for changed pixels ──
    output_buf = "\033[H";
    if (show_fps) {
        output_buf += "\033[mFPS:";
        append_uint(output_buf, fps);
        output_buf += ' ';
    }

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
                output_buf += "\033[";
                append_uint(output_buf, y + 1);
                output_buf += ";1H";
            }
            // Run-length: consecutive pixels with the same color (changed or not)
            int run = 1;
            while (x + run < w) {
                const Vec3& nxt = buffer[static_cast<size_t>(x + run) + static_cast<size_t>(y) * static_cast<size_t>(w)];
                if (nxt.x != cur.x || nxt.y != cur.y || nxt.z != cur.z) break;
                run++;
            }
            output_buf += "\033[";
            append_uint(output_buf, x * 2 + 1);
            output_buf += "G\033[48;2;";
            append_uint(output_buf, cur.x > 255 ? 255 : cur.x < 0 ? 0 : cur.x);
            output_buf += ';';
            append_uint(output_buf, cur.y > 255 ? 255 : cur.y < 0 ? 0 : cur.y);
            output_buf += ';';
            append_uint(output_buf, cur.z > 255 ? 255 : cur.z < 0 ? 0 : cur.z);
            output_buf += 'm';
            append_spans(output_buf, run * 2);
            x += run;
        }
    }

    prev_buffer = buffer;
}

void Screen::calculate_fps(double frame_time_ms) {
    // Batch-average every 10 frames, update display once per second
    static int batch_count = 0;
    static double batch_ms = 0.0;

    batch_ms += frame_time_ms;
    if (++batch_count >= 10) {
        double avg_fps = 1000.0 / (batch_ms / 10.0);
        if (avg_fps < fps_min) fps_min = avg_fps;
        if (avg_fps > fps_max) fps_max = avg_fps;

        // Update display every second
        static auto last_update = std::chrono::high_resolution_clock::now();
        auto now = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(now - last_update).count();
        if (elapsed >= 1.0) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "fps: %.0f/%.0f", fps_min, fps_max);
            fps_display = buf;
            fps_min = 1e9;
            fps_max = 0;
            last_update = now;
        }

        batch_ms = 0.0;
        batch_count = 0;
    }
}

void Screen::apply_ssaa() {
    // Downsample 2×2 blocks to 1 pixel �?inlined for speed, no lambda/ptr-to-member overhead
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
                // Horizontal edge �?blend vertically
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
                // Vertical edge �?blend horizontally
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

    // First frame or size mismatch �?initialize history
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

            // Luminance-based AABB from 3×3 neighborhood (per-channel AABB is too sensitive)
            float luma[9]; int li = 0;
            int min_r = 255, max_r = 0;
            int min_g = 255, max_g = 0;
            int min_b = 255, max_b = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    const int nx = x + dx, ny = y + dy;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                    const Vec3& n = buffer[nx + ny * width];
                    if (n.x < min_r) min_r = n.x;
                    if (n.x > max_r) max_r = n.x;
                    if (n.y < min_g) min_g = n.y;
                    if (n.y > max_g) max_g = n.y;
                    if (n.z < min_b) min_b = n.z;
                    if (n.z > max_b) max_b = n.z;
                    luma[li++] = 0.299f * n.x + 0.587f * n.y + 0.114f * n.z;
                }
            }

            // Softened clamp: lerp history toward clamped value
            float luma_min = luma[0], luma_max = luma[0];
            for (int i = 1; i < li; i++) {
                if (luma[i] < luma_min) luma_min = luma[i];
                if (luma[i] > luma_max) luma_max = luma[i];
            }
            float hist_luma = 0.299f * history.x + 0.587f * history.y + 0.114f * history.z;
            float clamp_factor = 1.0f;
            if (hist_luma < luma_min || hist_luma > luma_max) {
                // History is outside neighborhood → reduce its influence
                clamp_factor = 0.3f;
            }

            Vec3 clamped;
            clamped.x = clamp(history.x, min_r, max_r);
            clamped.y = clamp(history.y, min_g, max_g);
            clamped.z = clamp(history.z, min_b, max_b);
            // Blend: 50% history, 50% current
            constexpr float BLEND = 0.5f;
            aa_scratch[idx] = Vec3(
                static_cast<int>(lerp(static_cast<float>(clamped.x),
                    static_cast<float>(current.x), BLEND)),
                static_cast<int>(lerp(static_cast<float>(clamped.y),
                    static_cast<float>(current.y), BLEND)),
                static_cast<int>(lerp(static_cast<float>(clamped.z),
                    static_cast<float>(current.z), BLEND))
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
    platform::console_write("\x1b[?2026h", 8);
    platform::console_write(output_buf.data(), output_buf.size());
    platform::console_write("\x1b[?2026l", 8);
}


