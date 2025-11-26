
#ifndef PROJECT_CAMERA_H
#define PROJECT_CAMERA_H
#pragma once
#include "math_utils.h"
#include "mesh.h"
#include <windows.h>

class Screen;

class Camera {
public:
    Camera();
    void update(const Screen& screen);
    void load(Screen& screen, const Mesh& mesh) const;
    void controller();

    Vec4 pos;
    float a_x, a_y;
    POINT p1, p2;

private:
    float width, height, n, f, n2w, n2h;
    double fov_v, fov_h;

    static std::vector<Vec4> near_clip(const Vec4& a, const Vec4& b);
    static void division(Vec4& i);
};
#endif //PROJECT_CAMERA_H