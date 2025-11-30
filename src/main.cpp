#include "render.h"
#include "rubik_cube.h"

[[noreturn]] int main() {
    auto cube = RubikCube(5);
    Renderer r;

    r.toggle_fps();
    r.set_aa(SSAA);
    const ID rubik_cube_id = r.add_meshes(cube.meshes);
    r.set_camera_pos(Vec4(0, 0, -10, 1));
    r.launch(2);

    const auto rubik_indices = cube.block_indices;
    const auto rubik_mesh = r.operate_meshes(rubik_cube_id);
    while (true) {
        r.update();
        // RubikCube::control(rubik_indices, rubik_mesh);
        r.init_controller();
        Sleep(10);
    }
}