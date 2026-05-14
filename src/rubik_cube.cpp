#include "rubik_cube.h"
#include "transform.h"
#include "mesh.h"
#include "platform.h"

RubikCube::RubikCube(const float length) {
    const float len = length / 3;
    for (int i = 0; i < 27; ++i) {
        meshes.emplace_back(gen_block(Vec4(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1, 1), len));
        block_indices[i] = Vec3(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1);
    }
}

Mesh RubikCube::gen_block(const Vec4& pos, const float len) {
    constexpr Material material = Material{ 0.4f, 0.8f, 0.3f };
    const Vec3 color[6] = {
        Vec3(255,0,0),    // 红 — back  (z-)
        Vec3(0,255,0),    // 绿 — right (x+)
        Vec3(255,88,0),   // 橙 — front (z+)
        Vec3(0,0,255),    // 蓝 — left  (x-)
        Vec3(255,255,0),  // 黄 — top   (y+)
        Vec3(255,255,255),// 白 — bot   (y-)
    };

    const Vec4 center = pos * len * 1.04f;
    const float r = len / 2;

    // 8 顶点标准立方体
    const std::vector<Vec4> vertices = {
        {center.x - r, center.y - r, center.z - r, 1},
        {center.x + r, center.y - r, center.z - r, 1},
        {center.x + r, center.y + r, center.z - r, 1},
        {center.x - r, center.y + r, center.z - r, 1},
        {center.x - r, center.y - r, center.z + r, 1},
        {center.x + r, center.y - r, center.z + r, 1},
        {center.x + r, center.y + r, center.z + r, 1},
        {center.x - r, center.y + r, center.z + r, 1},
    };

    // 6 个面的索引（每个面 2 个三角形 = 6 个索引）
    static const int face_idx[6][6] = {
        {0, 1, 2, 2, 3, 0},  // 0: back  (z-)
        {1, 5, 6, 6, 2, 1},  // 1: right (x+)
        {5, 4, 7, 7, 6, 5},  // 2: front (z+)
        {4, 0, 3, 3, 7, 4},  // 3: left  (x-)
        {3, 2, 6, 6, 7, 3},  // 4: top   (y+)
        {1, 0, 4, 4, 5, 1},  // 5: bot   (y-)
    };

    // 面可见性：邻接面存在时隐藏（在 3x3x3 中只有最外层面可见）
    const bool visible[6] = {
        pos.z == -1,  // back
        pos.x == 1,   // right
        pos.z == 1,   // front
        pos.x == -1,  // left
        pos.y == 1,   // top
        pos.y == -1,  // bot
    };

    // 每个面的颜色贴图（外表面用彩色，内部隐藏面实际不会被渲染但留灰）
    const Vec3 face_colors[6] = {
        pos.z == -1 ? color[0] : Vec3(200,200,200),
        pos.x == 1  ? color[1] : Vec3(200,200,200),
        pos.z == 1  ? color[2] : Vec3(200,200,200),
        pos.x == -1 ? color[3] : Vec3(200,200,200),
        pos.y == 1  ? color[4] : Vec3(200,200,200),
        pos.y == -1 ? color[5] : Vec3(200,200,200),
    };

    // 收集所有 6 个面的索引和颜色（外部面彩色，内部面灰色——旋转时内部面也会暴露）
    std::vector<int> indices;
    std::vector<Vec3> mesh_colors;
    for (int f = 0; f < 6; f++) {
        for (int j = 0; j < 6; j++) {
            indices.push_back(face_idx[f][j]);
            mesh_colors.push_back(face_colors[f]);
        }
    }

    return { center, vertices, indices, mesh_colors, material };
}

void RubikCube::control(Vec3* blocks, const std::vector<Mesh*> &meshes) {
    // axis_base tracks the world axes as the scene is rotated by mouse
    static Vec4 axis_base[3] = {
        Vec4(1, 0, 0, 0),
        Vec4(0, 1, 0, 0),
        Vec4(0, 0, 1, 0)
    };

    // ── Non-blocking face-rotation animation state ────────────────────────
    static bool animating = false;
    static int anim_face;          // which face is rotating (0-5)
    static std::vector<int> anim_blk;
    static int anim_frames = 0;
    static constexpr int TOTAL_FRAMES = 9;

    // Start a new rotation from keyboard input (only if not already animating)
    if (!animating && platform::kbhit()) {
        Vec4 waxis;         // world-axis for block positions
        std::vector<int> blk;
        int face = -1;
        bool rotate = false;

        switch (platform::getch()) {
            case 'f': // 前面
                waxis = Vec4(0, 0, -1, 0); face = 0;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].z == -1) { blk.push_back(i); rotate = true; }
                break;
            case 'b': // 后面
                waxis = Vec4(0, 0, 1, 0); face = 1;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].z == 1) { blk.push_back(i); rotate = true; }
                break;
            case 'l': // 左面
                waxis = Vec4(-1, 0, 0, 0); face = 2;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].x == -1) { blk.push_back(i); rotate = true; }
                break;
            case 'r': // 右面
                waxis = Vec4(1, 0, 0, 0); face = 3;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].x == 1) { blk.push_back(i); rotate = true; }
                break;
            case 'd': // 下面
                waxis = Vec4(0, -1, 0, 0); face = 4;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].y == -1) { blk.push_back(i); rotate = true; }
                break;
            case 'u': // 上面
                waxis = Vec4(0, 1, 0, 0); face = 5;
                for (int i = 0; i < 27; ++i)
                    if (blocks[i].y == 1) { blk.push_back(i); rotate = true; }
                break;
            default:
                break;
        }

        if (rotate) {
            for (int i : blk)
                Transform::rotate(blocks[i], waxis, pi/2);
            animating = true;
            anim_face = face;
            anim_blk = std::move(blk);
            anim_frames = TOTAL_FRAMES;
        }
    }

    // Apply one animation step per frame (non-blocking, pi/18 × 9 = 90°)
    // Derive the scene-relative axis from axis_base each frame so mouse
    // drags during the animation stay in sync with the scene orientation.
    if (animating) {
        static const Vec4 face_axis[6] = {
            Vec4(0, 0, -1, 0),  // 0: back   axis_base[2] * -1
            Vec4(1, 0, 0, 0),   // 1: right  axis_base[0]
            Vec4(0, 0, 1, 0),   // 2: front  axis_base[2]
            Vec4(-1, 0, 0, 0),  // 3: left   axis_base[0] * -1
            Vec4(0, 1, 0, 0),   // 4: top    axis_base[1]
            Vec4(0, -1, 0, 0),  // 5: bottom axis_base[1] * -1
        };
        // Transform world-space face axis → scene-space via axis_base rotation matrix
        const Vec4& b = face_axis[anim_face];
        Vec4 scene_axis(
            b.x * axis_base[0].x + b.y * axis_base[1].x + b.z * axis_base[2].x,
            b.x * axis_base[0].y + b.y * axis_base[1].y + b.z * axis_base[2].y,
            b.x * axis_base[0].z + b.y * axis_base[1].z + b.z * axis_base[2].z,
            0
        );
        for (const int i : anim_blk)
            Transform::rotate(*meshes[i], scene_axis, pi / 18);
        if (--anim_frames <= 0)
            animating = false;
    }

        if (rotate) {
            // Logical positions use WORLD axes so face detection stays correct
            for (int i : blk)
                Transform::rotate(blocks[i], waxis, pi/2);
            animating = true;
            anim_axis = axis;      // scene-relative for mesh animation
            anim_blk = std::move(blk);
            anim_frames = TOTAL_FRAMES;
        }
    }

    // Apply one animation step per frame (non-blocking, pi/18 × 9 = 90°)
    if (animating) {
        for (const int i : anim_blk)
            Transform::rotate(*meshes[i], anim_axis, pi / 18);
        if (--anim_frames <= 0)
            animating = false;
    }

    // ── Mouse-controlled scene rotation ───────────────────────────────────
    static int p1x = 0, p1y = 0, p2x = 0, p2y = 0;
    platform::get_cursor_pos(p1x, p1y);
    const float angel_x = (p2x - p1x) * pi / 360.0f;
    const float angel_y = (p2y - p1y) * pi / 360.0f;
    p2x = p1x; p2y = p1y;

    if (platform::key_down(VK_RBUTTON)) {
        for (auto& axis : axis_base) {
            Transform::rotate(axis, Vec4(0, 1, 0, 0), angel_x);
            Transform::rotate(axis, Vec4(1, 0, 0, 0), angel_y);
        }
        for (auto& mesh : meshes) {
            Transform::rotate(*mesh, Vec4(0, 1, 0, 0), angel_x);
            Transform::rotate(*mesh, Vec4(1, 0, 0, 0), angel_y);
        }
    }
}