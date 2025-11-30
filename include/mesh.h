
#ifndef PROJECT_MESH_H
#define PROJECT_MESH_H
#include "math_utils.h"
#include <vector>

struct Mesh {
    Vec4 center;
    std::vector<Vec4> vertices;
    std::vector<int> indices;
    std::vector<Vec3> colors;

    static Mesh Cube(const Vec4& center, const float& r, const std::vector<Vec3>& colors);
    static Mesh Plane(const Vec4& center, const float& r, const std::vector<Vec3>& colors);
};
#endif //PROJECT_MESH_H