
#ifndef PROJECT_MESH_H
#define PROJECT_MESH_H
#include "math_utils.h"
#include "material.h"
#include <vector>

struct Mesh {
    Vec4 center;
    std::vector<Vec4> vertices;
    std::vector<int> indices;
    std::vector<Vec3> colors;
    Material material;

    static Mesh Cube(const Vec4& center, const float& r, const std::vector<Vec3>& colors, const Material& material );
    static Mesh Plane(const Vec4& center, const float& r, const std::vector<Vec3>& colors, const Material& material );
};
#endif //PROJECT_MESH_H