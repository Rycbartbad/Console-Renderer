#include "camera.h"
#include "screen.h"
#include "transform.h"
#include "graphics.h"
#include <windows.h>
#include <cmath>
#include <variant>

Camera::Camera() {
    width = height = 1;
    n = 1;
    f = 100;
    pos = Vec4(0, 0, 0, 1);
    a_x = a_y = 0;
    GetCursorPos(&p1);
    p2 = p1;
}

void Camera::update_from(const Screen& screen) {
    width = screen.width / static_cast<float>(screen.height);
}

void Camera::load(Screen& screen, const Mesh& mesh, const std::vector<Light>& lights) const {
    std::vector<Vec4> vertices;
    for (auto& i : mesh.vertices) {
        vertices.emplace_back(i.x * n * 2 / width, -i.y * n * 2, (i.z * (f + n) - 2 * n * f) / (f - n), i.z);
    }

    for (int i = 0; i + 2 < mesh.indices.size(); i += 3) {
        Vec3 color = mesh.colors[i];
            
        if (vertices[mesh.indices[i]].z < 1 && vertices[mesh.indices[i + 1]].z < 1 && vertices[mesh.indices[i + 2]].z < 1) 
            continue;
            
        std::vector<Vec4> clipped_vertices = near_clip(vertices[mesh.indices[i]], vertices[mesh.indices[i + 1]]);
        for (auto& j : near_clip(vertices[mesh.indices[i + 1]], vertices[mesh.indices[i + 2]])) {
            clipped_vertices.emplace_back(j);
        }
        for (auto& j : near_clip(vertices[mesh.indices[i + 2]], vertices[mesh.indices[i]])) {
            clipped_vertices.emplace_back(j);
        }
        auto before_div = clipped_vertices;
        for (auto& v : before_div) {
            v.x *= width / n / 2;
            v.y /= n * -2;
            v.z = v.w;
            v.w = 1;
        }
        if (!clipped_vertices.empty()) {
            division(clipped_vertices[0]);
            division(clipped_vertices[1]);
            const auto O = Vec2(clipped_vertices[0].x * 0.5 * screen.width, clipped_vertices[0].y * 0.5 * screen.height);

            for (int j = 1; j + 1 < clipped_vertices.size(); j++) {
                division(clipped_vertices[j + 1]);
                if (clipped_vertices[0].x < 0 && clipped_vertices[j].x < 0 && clipped_vertices[j + 1].x < 0) continue;
                if (clipped_vertices[0].x >= screen.width && clipped_vertices[j].x >= screen.width && clipped_vertices[j + 1].x >= screen.width) continue;
                if ((clipped_vertices[j].x - clipped_vertices[0].x) * (clipped_vertices[j + 1].y - clipped_vertices[0].y) - 
                    (clipped_vertices[j].y - clipped_vertices[0].y) * (clipped_vertices[j + 1].x - clipped_vertices[0].x) > 0)
                    continue;
                    
                const Vec4 v[3] = { before_div[0], before_div[j], before_div[j + 1] };
                Triangle::scan_draw(screen, O,
                    Vec2(clipped_vertices[j].x * 0.5 * screen.width, clipped_vertices[j].y * 0.5 * screen.height),
                    Vec2(clipped_vertices[j + 1].x * 0.5 * screen.width, clipped_vertices[j + 1].y * 0.5 * screen.height), 
                    color, mesh.material, lights, v, dir);
            }
        }
    }
}

std::vector<Vec4> Camera::near_clip(const Vec4& a, const Vec4& b) {
    if (a.z >= -a.w) {
        if (b.z >= -b.w) {
            return { b };
        }
        const float da = a.z + a.w;
        const float db = b.z + b.w;
        Vec4 insert = b * (da / (da - db)) - a * (db / (da - db));
        return { insert };
    }
    if (b.z >= -b.w) {
        const float da = a.z + a.w;
        const float db = b.z + b.w;
        Vec4 insert = a * (db / (db - da)) - b * (da / (db - da));
        return { insert, b };
    }
    return {};
}

void Camera::division(Vec4& i) {
    if (std::fabs(i.w) > 0.00001) {
        i.x = i.x / i.w + 1;
        i.y = i.y / i.w + 1;
        // i.z = i.z / i.w;
    }
}

void Camera::controller() {
    constexpr float speed = 0.1f;
    if (GetAsyncKeyState('W') & 0x8000)
        Transform::translate(pos, Vec4(sin(a_x), 0, cos(a_x), 0) * speed);
    if (GetAsyncKeyState('S') & 0x8000)
        Transform::translate(pos, Vec4(-sin(a_x), 0, -cos(a_x), 0) * speed);
    if (GetAsyncKeyState('A') & 0x8000)
        Transform::translate(pos, Vec4(-cos(a_x), 0, sin(a_x), 0) * speed);
    if (GetAsyncKeyState('D') & 0x8000)
        Transform::translate(pos, Vec4(cos(a_x), 0, -sin(a_x), 0) * speed);
    if (GetAsyncKeyState('Q') & 0x8000)
        Transform::translate(pos, Vec4(0, 1, 0, 0) * speed);
    if (GetAsyncKeyState('E') & 0x8000)
        Transform::translate(pos, Vec4(0, -1, 0, 0) * speed);

    GetCursorPos(&p1);
    const float angel_x = (p1.x - p2.x) * pi / 360.0f;
    const float angel_y = (p1.y - p2.y) * pi / 360.0f;
    p2 = p1;
    
    if (a_x + angel_x > pi * 2)
        a_x += angel_x - pi * 2;
    else if (a_x + angel_x < 0)
        a_x += angel_x + pi * 2;
    else 
        a_x += angel_x;

    if (a_y + angel_y < pi / 2 && a_y + angel_y > -pi / 2) {
        a_y += angel_y;
    }
    dir = get_dir();
}

Vec4 Camera::get_dir() const {
    auto dir = Vec4(0,0,1,0);
    Transform::rotate(dir, Vec4(0, 0, 1, 0), a_y);
    Transform::rotate(dir, Vec4(0, 1, 0, 0), a_x);
    dir.normalize();
    return dir;
}
