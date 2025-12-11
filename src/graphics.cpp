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
                        , const Material& material, const std::vector<Light>& lights, const Vec2* vertices, const Vec4* v, const Vec4& dir, const Vec4& normal) {
    const int sign_x = b.x > a.x ? 1 : b.x < a.x ? -1 : 0;
    if (a.y > screen.height - 1 || a.y < 0 || a.x == b.x) return;
    Vec2 point = a;
    for (int i = 0; i <= abs(a.x - b.x); i++) {
        const double gamma = (vertices[0] - point).cross(vertices[1] - point);
        const double alpha = (vertices[1] - point).cross(vertices[2] - point);
        const double beta = (vertices[2] - point).cross(vertices[0] - point);
        const double z_1 = alpha / v[0].z / (alpha + beta + gamma) + beta / v[1].z / (alpha + beta + gamma) + gamma / v[2].z / (alpha + beta + gamma);
        Vec4 point_3 = (v[0] * alpha / v[0].z + v[1] * beta / v[1].z + v[2] * gamma / v[2].z) / (alpha / v[0].z + beta / v[1].z + gamma / v[2].z);
        auto _color = Vec3();
        for (const auto&[pos, l_color, intensity] : lights) {
            Vec4 L = pos - point_3;
            const float dis = pow(L.x * L.x + L.y * L.y + L.z * L.z, 0.5);
            float _intensity = intensity / (0.1 + 0.01 * dis + 0.1 * dis * dis);
            L.normalize();
            Vec4 half_vec = (L - pos) / 2;
            if (L * normal <= 0)
                half_vec = Vec4();
            else
                half_vec.normalize();
            _color = _color + (color.hadamard(l_color) * (material.k_ambient +
                max(L * normal, 0.0f) * material.k_diffuse * _intensity)  / 255 +
                l_color * _intensity * static_cast<float>(std::pow(max(half_vec * normal, 0.0f), 5) * material.k_specular));
        }
        screen.set_pixel(point.x, point.y, _color, 1 / z_1);
        point.x += sign_x;
    }
}

void Triangle::scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, 
                        const Vec3& color, const Material& material, const std::vector<Light>& lights, const Vec4* v, const Vec4& dir) {
    if ((a - b).cross(c - b) < 3) return;
    std::vector<Vec2> vecs = { a, b, c };
    Vec2 temp;
    
    if (vecs[1].y < vecs[2].y) {
        temp = vecs[2];
        vecs[2] = vecs[1];
        vecs[1] = temp;
    }
    if (vecs[0].y < vecs[1].y) {
        temp = vecs[0];
        vecs[0] = vecs[1];
        vecs[1] = temp;
    }
    if (vecs[1].y < vecs[2].y) {
        temp = vecs[2];
        vecs[2] = vecs[1];
        vecs[1] = temp;
    }
    Vec4 normal = (v[2] - v[0]).cross3(v[1] - v[0]);
    normal.normalize();
    if (vecs[0].y == vecs[2].y) return;
    const int x = (vecs[1].y - vecs[2].y) * (vecs[0].x - vecs[2].x) / static_cast<double>(vecs[0].y - vecs[2].y) + .5 + vecs[2].x;
    const Vec2 vertices[3] = { a, b, c };
    fast_draw(screen, Vec2(x, vecs[1].y), vecs[1], color, material, lights, vertices, v, dir, normal);
    
    float x_1bias, x_2bias;
    if (x > vecs[1].x) {
        x_1bias = .75f;
        x_2bias = .5f;
    } else {
        x_1bias = .5f;
        x_2bias = .75f;
    }
    
    float dy = vecs[0].y - vecs[1].y;
    for (int i = 0; i <= dy; i++) {
        fast_draw(screen, 
                  Vec2((vecs[0].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y + i),
                  Vec2((vecs[0].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y + i),
                  color, material, lights, vertices, v, dir, normal);
    }
    
    dy = vecs[1].y - vecs[2].y;
    for (int i = 1; i <= dy; i++) {
        fast_draw(screen,
                  Vec2((vecs[2].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y - i),
                  Vec2((vecs[2].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y - i),
                  color, material, lights, vertices, v, dir, normal);
    }
}

void Triangle::scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& color) {
    std::vector<Vec2> vecs = { a, b, c };
    Vec2 temp;

    if (vecs[1].y < vecs[2].y) {
        temp = vecs[2];
        vecs[2] = vecs[1];
        vecs[1] = temp;
    }
    if (vecs[0].y < vecs[1].y) {
        temp = vecs[0];
        vecs[0] = vecs[1];
        vecs[1] = temp;
    }
    if (vecs[1].y < vecs[2].y) {
        temp = vecs[2];
        vecs[2] = vecs[1];
        vecs[1] = temp;
    }
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