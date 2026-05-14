#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"

[[noreturn]] int main() {
    Renderer r;
    r.set_camera_pos({ 0, 0, -5, 1 });
    r.set_aa(AA::SSAA);
    r.toggle_fps();

    RubikCube cube(10);
    ID mesh_id = r.add_meshes({cube.meshes});

    Light light{ { 0, 80, 0, 1 }, { 255, 255, 255 }, 1.0f };
    r.add_lights({ light });

    r.launch();
    while (true) {
        r.update();
        RubikCube::control(cube.block_indices, r.get_meshes(mesh_id));
    }
}
