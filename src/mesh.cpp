#include "mesh.h"

Mesh Mesh::Cube(const Vec4& center, float r, const std::vector<Vec3>& colors) {
    const std::vector<Vec4> vertices = {
        {center.x - r, center.y - r, center.z - r, 1}, {center.x + r, center.y - r, center.z - r, 1},
        {center.x + r, center.y + r, center.z - r, 1}, {center.x - r, center.y + r, center.z - r, 1},
        {center.x - r, center.y - r, center.z + r, 1}, {center.x + r, center.y - r, center.z + r, 1},
        {center.x + r, center.y + r, center.z + r, 1}, {center.x - r, center.y + r, center.z + r, 1},
    };
    
    const std::vector<short> indices = { 
        0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5, 
        4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 1, 0, 4, 4, 5, 1 
    };
    
    return { center, vertices, colors, indices };
}

Mesh Mesh::Plane(const Vec4& center, float r, const std::vector<Vec3>& colors) {
    const std::vector<Vec4> vertices = {
        {center.x - r, center.y, center.z - r, 1}, {center.x + r, center.y, center.z - r, 1},
        {center.x + r, center.y, center.z + r, 1}, {center.x - r, center.y, center.z + r, 1}
    };
    const std::vector<short> indices = { 0, 1, 2, 2, 3, 0 };
    return { center, vertices, colors, indices };
}