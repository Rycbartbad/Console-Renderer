#include "mesh.h"

std::vector<Vec3> auto_color(const std::vector<Vec3>& colors, const int& size) {
    std::vector<Vec3> color;
    color.resize(size);
    for (int i = 0; i < size; i++) {
        if (colors.size() == 1)
            color[i] = colors[0];
        else if (colors.size() * 3 == size)
            color[i] = colors[i / 3];
        else if (colors.size() * 6 == size)
            color[i] = colors[i / 6];
    }
    return color;
}

Mesh Mesh::Cube(const Vec4& center, const float& r, const std::vector<Vec3>& colors, const Material& material) {
    const std::vector<Vec4> vertices = {
        {center.x - r, center.y - r, center.z - r, 1}, {center.x + r, center.y - r, center.z - r, 1},
        {center.x + r, center.y + r, center.z - r, 1}, {center.x - r, center.y + r, center.z - r, 1},
        {center.x - r, center.y - r, center.z + r, 1}, {center.x + r, center.y - r, center.z + r, 1},
        {center.x + r, center.y + r, center.z + r, 1}, {center.x - r, center.y + r, center.z + r, 1},
    };
    
    const std::vector<int> indices = {
        0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5, 
        4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 1, 0, 4, 4, 5, 1 
    };
    
    return { center, vertices, indices, auto_color(colors, indices.size()), material };
}

Mesh Mesh::Plane(const Vec4& center, const float& r, const std::vector<Vec3>& colors, const Material& material ) {
    const std::vector<Vec4> vertices = {
        {center.x - r, center.y, center.z - r, 1}, {center.x + r, center.y, center.z - r, 1},
        {center.x + r, center.y, center.z + r, 1}, {center.x - r, center.y, center.z + r, 1}
    };
    const std::vector<int> indices = { 0, 1, 2, 2, 3, 0 };
    return { center, vertices, indices, auto_color(colors, indices.size()), material };
}