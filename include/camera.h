
#ifndef PROJECT_CAMERA_H
#define PROJECT_CAMERA_H
#include "math_utils.h"
#include "mesh.h"
#include "light.h"
#include "graphics.h"
#include <windows.h>

class Camera {
public:
    Camera();
    void update_from(const Screen& screen);
    void load(Screen& screen, const Mesh& mesh, const std::vector<Light>& lights,
              const Tile* clip_tile = nullptr, const Vec4* proj_verts = nullptr) const;
    void controller();
    void set_jitter(float jx, float jy);
    [[nodiscard]] Vec4 get_dir() const;
    // Pre-project all mesh vertices for one frame (avoids per-tile re-projection)
    void pre_project_verts(const std::vector<Mesh>& meshes, std::vector<Vec4>& out) const;

    Vec4 pos;
    float a_x, a_y;
    POINT p1, p2;
    Vec4 dir;
    float width = 1, height = 1;  // projection params (aspect ratio)
    float n = 1, f = 100;         // near/far plane

private:
    float jitter_x = 0, jitter_y = 0;

    // Returns number of vertices (0-2) written to out[]
    static int near_clip(const Vec4& a, const Vec4& b, Vec4 out[2]);
    static void division(Vec4& i);
};
#endif //PROJECT_CAMERA_H