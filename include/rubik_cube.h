
#ifndef PROJECT_RUBIK_CUBE_H
#define PROJECT_RUBIK_CUBE_H
#include "math_utils.h"
#include "mesh.h"
#include "transform.h"
#include <vector>

class RubikCube {
public:
    explicit RubikCube(float length);
    static void control(Vec3* blocks, std::vector<Mesh>& meshes);

    Vec3 blocks[27];
    std::vector<Mesh> meshes;

private:
    static Mesh gen_block(const Vec4& pos, float len);
};
#endif //PROJECT_RUBIK_CUBE_H