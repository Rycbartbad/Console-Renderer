#include "render.h"
#include "rubik_cube.h"
#include "platform.h"

[[noreturn]] int main() {
    auto cube = RubikCube(5);
    Renderer r;

    r.set_aa(SSAA);
    const ID rubik_cube_id = r.add(cube.meshes);
    auto light = Light{ Vec4(0, 80, 0, 1), Vec3(255,255,255), 800 };
    r.add({ light });
    r.set_camera_pos(Vec4(0, 0, -10, 1));
    r.launch();

    const auto rubik_indices = cube.block_indices;
    const auto rubik_mesh = r.operate_meshes(rubik_cube_id);
    while (true) {
        r.update();
        RubikCube::control(rubik_indices, rubik_mesh);
        platform::sleep_ms(10);
    }
}