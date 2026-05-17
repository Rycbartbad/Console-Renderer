#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"

[[noreturn]] int main() {
    Renderer r;
    r.set_camera_pos({ 0, 10, -20, 1 });
    r.set_aa(AA::SSAA);
    r.set_target_fps(60);
    r.toggle_fps();

    auto meshes = Mesh::LoadOBJ("src/test.obj");
    for (auto& m : meshes)
        Transform::scale(m, 2);
    if (meshes.empty())
        meshes.push_back(Mesh::Cube({ 0, 0, 0, 1 }, 5, { Vec3(200, 200, 200) },
                                     Material{ 0.4f, 0.8f, 0.3f }));
    ID mesh_id = r.add_meshes(meshes);

    r.add_lights({
        { { 0, 80, 0, 1 },  { 255, 255, 255 }, 1500 },
    });

    r.launch();
    while (true) {
        r.controller();
        r.update();
    }
}
