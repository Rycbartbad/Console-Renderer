#include "graphics.h"
#include "screen.h"
#include <cmath>

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

void Line::fast_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color, 
                    const Vec2* vertices, const float* v_z, float f, float n) {
    const int sign_x = b.x > a.x ? 1 : b.x < a.x ? -1 : 0;
    if (a.y > screen.height - 1 || a.y < 0 || a.x == b.x) return;
    Vec2 point = a;
    for (int i = 0; i <= abs(a.x - b.x); i++) {
        const int gamma = (vertices[0] - point).cross(vertices[1] - point);
        const int alpha = (vertices[1] - point).cross(vertices[2] - point);
        const int beta = (vertices[2] - point).cross(vertices[0] - point);
        const float z = alpha * v_z[0] / (alpha + beta + gamma) + beta * v_z[1] / (alpha + beta + gamma) + gamma * v_z[2] / (alpha + beta + gamma);
        screen.set_pixel(point.x, point.y, color, z);
        point.x += sign_x;
    }
}

void Triangle::scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, 
                        const Vec3& color, const float* v_z, float f, float n) {
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
    
    if (vecs[0].y == vecs[2].y) return;
    const int x = (vecs[1].y - vecs[2].y) * (vecs[0].x - vecs[2].x) / static_cast<double>(vecs[0].y - vecs[2].y) + .5 + vecs[2].x;
    const Vec2 vertices[3] = { a, b, c };
    fast_draw(screen, Vec2(x, vecs[1].y), vecs[1], color, vertices, v_z, f, n);
    
    float x_1bias, x_2bias;
    if (x > vecs[1].x) {
        x_1bias = .48f;
        x_2bias = .5f;
    } else {
        x_1bias = .5f;
        x_2bias = .48f;
    }
    
    float dy = vecs[0].y - vecs[1].y;
    for (int i = 0; i <= dy; i++) {
        fast_draw(screen, 
            Vec2((vecs[0].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y + i),
            Vec2((vecs[0].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y + i),
            color, vertices, v_z, f, n);
    }
    
    dy = vecs[1].y - vecs[2].y;
    for (int i = 1; i <= dy; i++) {
        fast_draw(screen,
            Vec2((vecs[2].x * i + x * (dy - i)) / dy + x_1bias, vecs[1].y - i),
            Vec2((vecs[2].x * i + vecs[1].x * (dy - i)) / dy + x_2bias, vecs[1].y - i),
            color, vertices, v_z, f, n);
    }
}