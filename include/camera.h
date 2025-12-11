
#ifndef PROJECT_CAMERA_H
#define PROJECT_CAMERA_H
#include "math_utils.h"
#include "mesh.h"
#include "light.h"
#include "graphics.h"
#include <windows.h>

class Screen;

class Camera {
public:
    Camera();
    void update_from(const Screen& screen);
    void load(Screen& screen, const Mesh& mesh, const std::vector<Light>& lights) const;
    void controller();
    [[nodiscard]] Vec4 get_dir() const;

    Vec4 pos;
    float a_x, a_y;
    POINT p1, p2;
    Vec4 dir;
private:
    float width, height, n, f;

    static std::vector<Vec4> near_clip(const Vec4& a, const Vec4& b);
    static void division(Vec4& i);
};
#endif //PROJECT_CAMERA_H