
#ifndef PROJECT_TRANSFORM_H
#define PROJECT_TRANSFORM_H
#include "math_utils.h"
#include "mesh.h"

class Transform {
public:
    static void translate(Mesh& mesh, const Vec4& translation);
    static void translate(Vec4& pos, const Vec4& translation);
    static void rotate(Mesh& mesh, const Vec4& center, Vec4 axis, float angle);
    static void rotate(Mesh& mesh, Vec4 axis, float angle);

    static void rotate(Vec4& dir, Vec4 axis, float angle);
    static void rotate(Vec3& point, Vec4 axis, float angle);
};
#endif //PROJECT_TRANSFORM_H