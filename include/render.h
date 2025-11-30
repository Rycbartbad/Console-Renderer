
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
    void set_aa(AA);
    void set_camera_pos(const Vec4&);
    void init_controller();
    void launch(int);
    ID add_meshes(const std::vector<Mesh>&);
    std::vector<Mesh*> operate_meshes(ID);

private:
    [[noreturn]] void ShadeCycle() const;
    Camera camera;
    std::vector<Mesh> meshes;
    Screen screen;
    int mesh_num;
};
#endif //PROJECT_RENDER_H