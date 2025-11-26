#include "render.h"
#include "rubik_cube.h"

[[noreturn]] int main() {
    auto cube = RubikCube(5);
    Render render;

    render.toggle_fps();
    render.set_aa(SSAA);
    render.meshes = cube.meshes;
    render.set_camera_pos(Vec4(0, 0, -10, 1));
    render.launch(2);

    while (true) {
        render.update();
        RubikCube::control(cube.blocks, render.meshes);
        Sleep(10);
    }
}