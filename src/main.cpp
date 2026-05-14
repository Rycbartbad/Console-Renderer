#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"

[[noreturn]] int main() {
    Renderer r;
    r.set_camera_pos({ 0, 0, -20, 1 });
    r.set_aa(AA::SSAA);
    r.toggle_fps();

    RubikCube cube(10);
    ID mesh_id = r.add_meshes(cube.meshes);

    std::vector<Light> lights = {
        { { 0, 80, 0, 1 },  { 255, 255, 255 }, 1500 },
        { { -40, 60, 40, 1 }, { 200, 180, 255 }, 800 },
        { { 40, 60, -40, 1 },  { 255, 200, 180 }, 800 },
        { { 0, 20, 60, 1 },    { 180, 255, 200 }, 600 },
        { { 0, 100, -20, 1 },  { 255, 200, 255 }, 600 },
    };
    r.add_lights(lights);

    r.launch();
    while (true) {
        r.update();
        RubikCube::control(cube.block_indices, r.get_meshes(mesh_id));
    }
}
