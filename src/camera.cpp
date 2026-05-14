#include "camera.h"
#include "screen.h"
#include "transform.h"
#include "graphics.h"
#include <cmath>
#include <variant>

Camera::Camera() {
    width = height = 1;
    n = 1;
    f = 100;
    pos = Vec4(0, 0, 0, 1);
    a_x = a_y = 0;
    platform::get_cursor_pos(p1x, p1y);
    p2x = p1x; p2y = p1y;
}

void Camera::update_from(const Screen& screen) {
    width = screen.width / static_cast<float>(screen.height);
}

void Camera::pre_project_verts(const std::vector<Mesh>& meshes, std::vector<Vec4>& out) const {
    out.clear();
    for (const auto& mesh : meshes) {
        for (const auto& src : mesh.vertices) {
            out.push_back(Vec4(
                src.x * n * 2 / width,
                -src.y * n * 2,
                (src.z * (f + n) - 2 * n * f) / (f - n),
                src.z
            ));
        }
    }
}

void Camera::load(Screen& screen, const Mesh& mesh, const std::vector<Light>& lights,
                  const Tile* clip_tile, const Vec4* proj_verts) const {
    // Pre-project all vertices (mesh.vertices is small — 8 for a cube)
    Vec4 vbuf[8];  // enough for any mesh in this app
    const int vn = static_cast<int>(mesh.vertices.size());
    if (proj_verts) {
        for (int vi = 0; vi < vn; vi++) vbuf[vi] = proj_verts[vi];
    } else {
        for (int vi = 0; vi < vn; vi++) {
            const auto& src = mesh.vertices[vi];
            vbuf[vi] = Vec4(src.x * n * 2 / width, -src.y * n * 2,
                            (src.z * (f + n) - 2 * n * f) / (f - n), src.z);
        }
    }

    const float sx = 0.5f * screen.width;
    const float sy = 0.5f * screen.height;

    // Fast mesh-level tile culling: compute screen AABB from projected vertices
    if (clip_tile && vn > 0) {
        float min_sx = 1e9f, max_sx = -1e9f;
        float min_sy = 1e9f, max_sy = -1e9f;
        for (int vi = 0; vi < vn; vi++) {
            if (vbuf[vi].z < 1) continue;  // behind near plane
            const float inv_w = 1.0f / vbuf[vi].w;
            float px = vbuf[vi].x * inv_w;
            float py = vbuf[vi].y * inv_w;
            float scx = (px + 1.0f) * sx;
            float scy = (py + 1.0f) * sy;
            if (scx < min_sx) min_sx = scx;
            if (scx > max_sx) max_sx = scx;
            if (scy < min_sy) min_sy = scy;
            if (scy > max_sy) max_sy = scy;
        }
        if (max_sx < clip_tile->x_start || min_sx >= clip_tile->x_end ||
            max_sy < clip_tile->y_start || min_sy >= clip_tile->y_end)
            return;  // mesh entirely outside this tile
    }

    for (int ti = 0; ti + 2 < static_cast<int>(mesh.indices.size()); ti += 3) {
        Vec3 color = mesh.colors[ti];

        const int ia = mesh.indices[ti], ib = mesh.indices[ti + 1], ic = mesh.indices[ti + 2];
        if (vbuf[ia].z < 1 && vbuf[ib].z < 1 && vbuf[ic].z < 1)
            continue;

        // Fast path: all vertices inside near plane → skip clipping entirely
        Vec4 cv[6]; int cv_n;
        if (vbuf[ia].z >= -vbuf[ia].w && vbuf[ib].z >= -vbuf[ib].w && vbuf[ic].z >= -vbuf[ic].w) {
            cv[0] = vbuf[ia]; cv[1] = vbuf[ib]; cv[2] = vbuf[ic]; cv_n = 3;
        } else {
            cv_n = 0;
            cv_n += near_clip(vbuf[ia], vbuf[ib], cv + cv_n);
            cv_n += near_clip(vbuf[ib], vbuf[ic], cv + cv_n);
            cv_n += near_clip(vbuf[ic], vbuf[ia], cv + cv_n);
        }

        if (cv_n < 3) continue;  // degenerate

        // before_div = clipped_vertices copy + perspective-scale
        Vec4 bd[6];
        for (int v = 0; v < cv_n; v++) {
            bd[v] = cv[v];
            bd[v].x *= width / n / 2;
            bd[v].y /= n * -2;
            bd[v].x += jitter_x * 2.0f / screen.width;
            bd[v].y += jitter_y * 2.0f / screen.height;
            bd[v].z = bd[v].w;
            bd[v].w = 1;
        }

        // Perspective division + screen-space mapping
        division(cv[0]); division(cv[1]);
        const float o_x = cv[0].x * sx;
        const float o_y = cv[0].y * sy;

        for (int j = 1; j + 1 < cv_n; j++) {
            division(cv[j + 1]);

            // Quick screen-bound reject
            if (cv[0].x < 0 && cv[j].x < 0 && cv[j + 1].x < 0) continue;
            if (cv[0].x >= 2.0f && cv[j].x >= 2.0f && cv[j + 1].x >= 2.0f) continue;
            // Backface cull
            if ((cv[j].x - cv[0].x) * (cv[j + 1].y - cv[0].y) -
                (cv[j].y - cv[0].y) * (cv[j + 1].x - cv[0].x) > 0)
                continue;

            // Compute screen-space coords once
            const float p0x = o_x, p0y = o_y;
            const float p1x = cv[j].x * sx, p1y = cv[j].y * sy;
            const float p2x = cv[j + 1].x * sx, p2y = cv[j + 1].y * sy;

            // ── Tile clip: conservative bounding box ──
            if (clip_tile) {
                int xp[3] = { static_cast<int>(p0x), static_cast<int>(p1x), static_cast<int>(p2x) };
                int yp[3] = { static_cast<int>(p0y), static_cast<int>(p1y), static_cast<int>(p2y) };
                int mnx = xp[0], mxx = xp[0] + 1;
                int mny = yp[0], mxy = yp[0] + 1;
                for (int v = 1; v < 3; v++) {
                    if (xp[v] < mnx) mnx = xp[v];
                    if (xp[v] + 1 > mxx) mxx = xp[v] + 1;
                    if (yp[v] < mny) mny = yp[v];
                    if (yp[v] + 1 > mxy) mxy = yp[v] + 1;
                }
                if (mxx < clip_tile->x_start || mnx >= clip_tile->x_end ||
                    mxy < clip_tile->y_start || mny >= clip_tile->y_end)
                    continue;
            }

            const Vec4 vert_attr[3] = { bd[0], bd[j], bd[j + 1] };
            Triangle::scan_draw(screen,
                Vec2(static_cast<int>(p0x), static_cast<int>(p0y)),
                Vec2(static_cast<int>(p1x), static_cast<int>(p1y)),
                Vec2(static_cast<int>(p2x), static_cast<int>(p2y)),
                color, mesh.material, lights, vert_attr, dir, clip_tile);
        }
    }
}

void Camera::set_jitter(const float jx, const float jy) {
    jitter_x = jx;
    jitter_y = jy;
}

int Camera::near_clip(const Vec4& a, const Vec4& b, Vec4 out[2]) {
    if (a.z >= -a.w) {
        if (b.z >= -b.w) {
            out[0] = b;
            return 1;
        }
        const float da = a.z + a.w;
        const float db = b.z + b.w;
        out[0] = b * (da / (da - db)) - a * (db / (da - db));
        return 1;
    }
    if (b.z >= -b.w) {
        const float da = a.z + a.w;
        const float db = b.z + b.w;
        out[0] = a * (db / (db - da)) - b * (da / (db - da));
        out[1] = b;
        return 2;
    }
    return 0;
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
    if (platform::key_down('W'))
        Transform::translate(pos, Vec4(sin(a_x), 0, cos(a_x), 0) * speed);
    if (platform::key_down('S'))
        Transform::translate(pos, Vec4(-sin(a_x), 0, -cos(a_x), 0) * speed);
    if (platform::key_down('A'))
        Transform::translate(pos, Vec4(-cos(a_x), 0, sin(a_x), 0) * speed);
    if (platform::key_down('D'))
        Transform::translate(pos, Vec4(cos(a_x), 0, -sin(a_x), 0) * speed);
    if (platform::key_down('Q'))
        Transform::translate(pos, Vec4(0, 1, 0, 0) * speed);
    if (platform::key_down('E'))
        Transform::translate(pos, Vec4(0, -1, 0, 0) * speed);

    platform::get_cursor_pos(p1x, p1y);
    const float angel_x = (p1x - p2x) * pi / 360.0f;
    const float angel_y = (p1y - p2y) * pi / 360.0f;
    p2x = p1x; p2y = p1y;
    
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
