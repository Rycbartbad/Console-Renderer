
#ifndef PROJECT_RENDER_H
#define PROJECT_RENDER_H
#include "math_utils.h"
#include "mesh.h"
#include "camera.h"
#include "screen.h"
#include <vector>

enum AA { SSAA };

struct ID {
    int begin;
    int end;
};

class Renderer{
public:
    Renderer();
    void update();
    void toggle_fps();
    void set_aa(AA aa);
    void set_camera_pos(const Vec4& pos);
    void launch(int threads);
    ID add_meshes(const std::vector<Mesh> &_meshes);
    std::vector<Mesh*> operate_meshes(ID id);

    Camera camera;
    std::vector<Mesh> meshes;

private:
    [[noreturn]] static void ShadeCycle(const std::vector<Mesh>& meshes, const Camera& camera,
                                   const bool& show_fps, const bool& SSAA, const int& width,
                                   const int& height, const bool& bias);

    bool show;
    Screen screen;
    int mesh_num;
};
#endif //PROJECT_RENDER_H