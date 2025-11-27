#include "render.h"
#include "rubik_cube.h"

[[noreturn]] int main() {
    auto cube = RubikCube(5);
    Renderer renderer;

    renderer.toggle_fps();
    renderer.set_aa(SSAA);
    ID rubik_cube_id = renderer.add_meshes(cube.meshes);
    renderer.set_camera_pos(Vec4(0, 0, -10, 1));
    renderer.launch(2);

    while (true) {
        renderer.update();
        RubikCube::control(cube.block_indices, renderer.operate_meshes(rubik_cube_id));
        Sleep(10);
    }
}