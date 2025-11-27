#include "transform.h"
#include "mesh.h"
#include <cmath>

void Transform::translate(Mesh& mesh, const Vec4& translation) {
    for (auto& v : mesh.vertices) {
        v = translation + v;
    }
    mesh.center = translation + mesh.center;
}

void Transform::translate(Vec4& pos, const Vec4& translation) {
    pos = pos + translation;
}

void Transform::rotate(Mesh& mesh, const Vec4& center, Vec4 axis, float angle) {
    axis.normalize();
    translate(mesh, center * (-1));
    for (auto& i : mesh.vertices) {
        i = i * cos(angle) + axis.cross3(i) * sin(angle) + axis * (axis * i) * (1 - cos(angle));
        i.w = 1;
    }
    translate(mesh, center);
}

void Transform::rotate(Mesh& mesh, Vec4 axis, float angle) {
    axis.normalize();
    for (auto& i : mesh.vertices) {
        i = i * cos(angle) + axis.cross3(i) * sin(angle) + axis * (axis * i) * (1 - cos(angle));
        i.w = 1;
    }
}

void Transform::rotate(Vec4& dir, Vec4 axis, float angle) {
    axis.normalize();
    dir = dir * cos(angle) + axis.cross3(dir) * sin(angle) + axis * (axis * dir) * (1 - cos(angle));
}

void Transform::rotate(Vec3& point, Vec4 axis, float angle) {
    axis.normalize();
    const auto tmp = Vec4(point.x, point.y, point.z, 1);
    point = (tmp * cos(angle) + axis.cross3(tmp) * sin(angle) + axis * (axis * tmp) * (1 - cos(angle))).to_vec3();
}