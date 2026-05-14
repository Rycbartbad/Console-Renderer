#include "graphics.h"
#include "screen.h"
#include "camera.h"

void Line::draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color) {
    bool shift = false;
    int dx = abs(b.x - a.x);
    int dy = abs(b.y - a.y);
    if (dx < dy) {
        const int temp = dx;
        dx = dy;
        dy = temp;
        shift = true;
    }
    const int sign_x = b.x > a.x ? 1 : b.x < a.x ? -1 : 0;
    const int sign_y = b.y > a.y ? 1 : b.y < a.y ? -1 : 0;
    Vec2 point = a;
    int e = -dx;
    for (int i = 0; i < dx; i++) {
        screen.set_pixel(point.x, point.y, color, 1);
        if (shift) {
            point.y += sign_y;
        } else {
            point.x += sign_x;
        }
        e += 2 * dy;
        if (e >= 0) {
            if (shift) {
                point.x += sign_x;
            } else {
                point.y += sign_y;
            }
            e -= 2 * dx;
        }
    }
    screen.set_pixel(point.x, point.y, color, 1);
}

void Line::fast_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color
                        , const Material& material, const std::vector<Light>& lights, const Vec2* vertices, const Vec4* v, const Vec4& dir, const Vec4& normal
                        , const Tile* scissor) {
    const int sign_x = b.x > a.x ? 1 : b.x < a.x ? -1 : 0;
    if (a.y > screen.height - 1 || a.y < 0 || a.x == b.x) return;

    // Incremental barycentrics: precompute edge derivatives
    // alpha = cross(B, C), beta = cross(C, A), gamma = cross(A, B)
    // Edge equation E(x,y) = C + y*Dy + x*Dx where:
    //   C = A.x*B.y - A.y*B.x
    //   Dx = A.y - B.y
    //   Dy = B.x - A.x
    const Vec2 &A = vertices[0], &B = vertices[1], &C = vertices[2];
    const int Dx_a = B.y - C.y, Dy_a = C.x - B.x;
    const int Dx_b = C.y - A.y, Dy_b = A.x - C.x;
    const int Dx_g = A.y - B.y, Dy_g = B.x - A.x;
    // Starting values at pixel 'a' (first pixel in scanline)
    int alpha = (B - a).cross(C - a);
    int beta  = (C - a).cross(A - a);
    int gamma = (A - a).cross(B - a);
    // Precompute per-vertex depth weights as floats
    const float iv0z = 1.0f / v[0].z, iv1z = 1.0f / v[1].z, iv2z = 1.0f / v[2].z;

    // ── Scissor: clip scanline to tile bounds ──
    const int span_len = abs(a.x - b.x);
    int i_start = 0, i_end = span_len;
    if (scissor) {
        const int sx0 = scissor->x_start;
        const int sx1 = scissor->x_end;
        // Y-range check
        if (a.y < scissor->y_start || a.y >= scissor->y_end) return;
        // Quick span-vs-scissor overlap test
        int span_l = a.x, span_r = b.x;
        if (span_l > span_r) std::swap(span_l, span_r);
        if (span_l >= sx1 || span_r < sx0) return;
        // Compute clipped iteration range
        if (sign_x > 0) {
            if (a.x < sx0) i_start = sx0 - a.x;
            if (b.x >= sx1) i_end = sx1 - 1 - a.x;
        } else {
            if (a.x >= sx1) i_start = a.x - (sx1 - 1);
            if (b.x < sx0) i_end = span_len - (sx0 - b.x);
        }
        if (i_start > i_end) return;
        // Advance barycentrics past skipped pixels
        alpha += Dx_a * sign_x * i_start;
        beta  += Dx_b * sign_x * i_start;
        gamma += Dx_g * sign_x * i_start;
    }
    const int iterations = i_end - i_start;

    // ── Tiled path: direct buffer access, no virtual dispatch ──
    if (scissor) {
        const int tile_w = scissor->x_end - scissor->x_start;
        const int tile_ox = scissor->x_start;
        const int tile_oy = scissor->y_start;
        Vec2 point = { a.x + i_start * sign_x, a.y };
        const float inv_light_count = 1.0f / static_cast<float>(lights.size());
        for (int i = 0; i <= iterations; i++) {
            const double ad = alpha, bd = beta, gd = gamma;
            const double sum_abc = ad + bd + gd;
            const float inv_denom = static_cast<float>(1.0 / (ad * iv0z + bd * iv1z + gd * iv2z));
            const float depth = static_cast<float>(sum_abc) * inv_denom;
            constexpr float DEPTH_BIAS = 2e-7f;  // prevents z-fighting on shared edges

            // Inlined depth test + pixel write (no bounds check — scissor already guarantees in-range)
            const size_t idx = static_cast<size_t>(point.x - tile_ox)
                             + static_cast<size_t>(point.y - tile_oy) * static_cast<size_t>(tile_w);
            if (screen.z_buffer[idx] == 0 || depth + DEPTH_BIAS < screen.z_buffer[idx]) {
                screen.z_buffer[idx] = depth;
                // Perspective-correct 3D position
                const Vec4 point_3 = (v[0] * static_cast<float>(ad * iv0z) +
                                      v[1] * static_cast<float>(bd * iv1z) +
                                      v[2] * static_cast<float>(gd * iv2z)) * inv_denom;
                auto _color = Vec3();
                for (const auto&[pos, l_color, intensity] : lights) {
                    Vec4 L = pos - point_3;
                    const float L_sq = L * L;
                    const float inv_dis = 1.0f / std::sqrt(L_sq);
                    const float dis = L_sq * inv_dis;
                    float _intensity = intensity / (0.1f + 0.01f * dis + 0.1f * L_sq);
                    const Vec4 L_dir = L * inv_dis;
                    // Specular: fast ndotl^power (view-independent, avoids extra sqrt for |point_3|)
                    const float ndotl = std::max(L_dir * normal, 0.0f);
                    const float spec = ndotl * ndotl * ndotl * ndotl * ndotl;
                    _color = _color + (color.hadamard(l_color) * (material.k_ambient +
                        ndotl * material.k_diffuse * _intensity) / 255 +
                        l_color * _intensity * (spec * material.k_specular)) * inv_light_count;
                }
                // Clamp to 255 (always ≥ 0 from the lighting math)
                screen.buffer[idx] = Vec3(
                    std::min(_color.x, 255),
                    std::min(_color.y, 255),
                    std::min(_color.z, 255));
            }
            point.x += sign_x;
            if (i < iterations) {
                alpha += Dx_a * sign_x;
                beta  += Dx_b * sign_x;
                gamma += Dx_g * sign_x;
            }
        }
    } else {
        const float inv_light_count = 1.0f / static_cast<float>(lights.size());
        Vec2 point = { a.x + i_start * sign_x, a.y };
        for (int i = 0; i <= iterations; i++) {
            const double ad = alpha, bd = beta, gd = gamma;
            const double sum_abc = ad + bd + gd;
            const float inv_denom = static_cast<float>(1.0 / (ad * iv0z + bd * iv1z + gd * iv2z));
            const float depth = static_cast<float>(sum_abc) * inv_denom;

            if (screen.depth_test(point.x, point.y, depth)) {
                const Vec4 point_3 = (v[0] * static_cast<float>(ad * iv0z) +
                                      v[1] * static_cast<float>(bd * iv1z) +
                                      v[2] * static_cast<float>(gd * iv2z)) * inv_denom;
                auto _color = Vec3();
                for (const auto&[pos, l_color, intensity] : lights) {
                    Vec4 L = pos - point_3;
                    const float L_sq = L * L;
                    const float inv_dis = 1.0f / std::sqrt(L_sq);
                    const float dis = L_sq * inv_dis;
                    float _intensity = intensity / (0.1f + 0.01f * dis + 0.1f * L_sq);
                    const Vec4 L_dir = L * inv_dis;
                    const float ndotl = std::max(L_dir * normal, 0.0f);
                    const float spec = ndotl * ndotl * ndotl * ndotl * ndotl;
                    _color = _color + (color.hadamard(l_color) * (material.k_ambient +
                        ndotl * material.k_diffuse * _intensity) / 255 +
                        l_color * _intensity * (spec * material.k_specular)) * inv_light_count;
                }
                screen.set_pixel(point.x, point.y, Vec3(
                    std::min(_color.x, 255),
                    std::min(_color.y, 255),
                    std::min(_color.z, 255)), depth);
            }
            point.x += sign_x;
            if (i < iterations) {
                alpha += Dx_a * sign_x;
                beta  += Dx_b * sign_x;
                gamma += Dx_g * sign_x;
            }
        }
    }
}

void Triangle::scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, 
                        const Vec3& color, const Material& material, const std::vector<Light>& lights, const Vec4* v, const Vec4& dir,
                        const Tile* scissor) {
    if ((a - b).cross(c - b) < 3) return;
    Vec2 vecs[3] = { a, b, c };

    if (vecs[1].y < vecs[2].y) std::swap(vecs[1], vecs[2]);
    if (vecs[0].y < vecs[1].y) std::swap(vecs[0], vecs[1]);
    if (vecs[1].y < vecs[2].y) std::swap(vecs[1], vecs[2]);
    Vec4 normal = (v[2] - v[0]).cross3(v[1] - v[0]);
    normal.normalize();
    if (vecs[0].y == vecs[2].y) return;
    const int x = (vecs[1].y - vecs[2].y) * (vecs[0].x - vecs[2].x) / static_cast<double>(vecs[0].y - vecs[2].y) + .5 + vecs[2].x;
    const Vec2 verts[3] = { a, b, c };
    fast_draw(screen, Vec2(x, vecs[1].y), vecs[1], color, material, lights, verts, v, dir, normal, scissor);
    
    float x_1bias, x_2bias;
    if (x > vecs[1].x) {
        x_1bias = .05f;
        x_2bias = -.05f;
    } else {
        x_1bias = -.05f;
        x_2bias = .05f;
    }
    
    float dy = vecs[0].y - vecs[1].y;
    for (int i = 0; i <= dy; i++) {
        fast_draw(screen, 
                  Vec2((vecs[0].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y + i),
                  Vec2((vecs[0].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y + i),
                  color, material, lights, verts, v, dir, normal, scissor);
    }
    
    dy = vecs[1].y - vecs[2].y;
    for (int i = 1; i <= dy; i++) {
        fast_draw(screen,
                  Vec2((vecs[2].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y - i),
                  Vec2((vecs[2].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y - i),
                  color, material, lights, verts, v, dir, normal, scissor);
    }
}

void Triangle::scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& color) {
    Vec2 vecs[3] = { a, b, c };

    if (vecs[1].y < vecs[2].y) std::swap(vecs[1], vecs[2]);
    if (vecs[0].y < vecs[1].y) std::swap(vecs[0], vecs[1]);
    if (vecs[1].y < vecs[2].y) std::swap(vecs[1], vecs[2]);
    if (vecs[0].y == vecs[2].y) return;
    const int x = (vecs[1].y - vecs[2].y) * (vecs[0].x - vecs[2].x) / static_cast<double>(vecs[0].y - vecs[2].y) + .5 + vecs[2].x;
    draw(screen, Vec2(x, vecs[1].y), vecs[1], color);

    float dy = vecs[0].y - vecs[1].y;
    for (int i = 0; i <= dy; i++) {
        draw(screen,
                  Vec2((vecs[0].x * i + x * (dy - i)) / dy, vecs[1].y + i),
                  Vec2((vecs[0].x * i + vecs[1].x * (dy - i)) / dy, vecs[1].y + i),
                  color);
    }

    dy = vecs[1].y - vecs[2].y;
    for (int i = 1; i <= dy; i++) {
        draw(screen,
                  Vec2((vecs[2].x * i + x * (dy - i)) / dy, vecs[1].y - i),
                  Vec2((vecs[2].x * i + vecs[1].x * (dy - i)) / dy, vecs[1].y - i),
                  color);
    }
}