#include "render.h"
#include "rubik_cube.h"

[[noreturn]] int main() {
    //auto cube = RubikCube(2);
    //auto light = Light{ Vec4(0,6,15,1), Vec3(255,255,255), 5 };
    //Renderer r;

    //r.toggle_fps();
    //r.set_aa(SSAA);
    //for (int i = 0; i < 10; i++) {
    //    for (int j = 0; j < 10; j++) {
    //        for (const auto& mesh : r.operate_meshes(r.add(cube.meshes))) {
    //            Transform::rotate(*mesh, Vec4(1, 0, 0, 0), pi / 10 * i);
    //            Transform::rotate(*mesh, Vec4(0, 0, 1, 0), pi / 10 * j);
    //            Transform::translate(*mesh, Vec4(i * 5 - 25, 0, j * 5 - 25, 0));
    //        }
    //    }
    //}
    //// const ID rubik_cube_id = r.add(cube.meshes);
    //const ID light_id = r.add({light});
    //r.set_camera_pos(Vec4(0, 0, -10, 1));
    //r.launch(2);

    //const auto rubik_indices = cube.block_indices;
    //// const auto rubik_mesh = r.operate_meshes(rubik_cube_id);
    //while (true) {
    //    r.update();
    //    for (const auto& _light : r.operate_lights(light_id)) {
    //        Transform::rotate(_light->pos, Vec4(0, 1, 0, 0), pi / 100);
    //    }
    //    // RubikCube::control(rubik_indices, rubik_mesh);
    //    r.init_controller();
    //    Sleep(10);
    //}

    Material m = { 0.3, 1.6, 0.5 };
    Mesh cube = Mesh::Cube(Vec4(0, 0, 0, 1), 4, { Vec3(164, 227, 240) }, m);
    Mesh plane = Mesh::Plane(Vec4(0, -5, 0, 1), 100, { Vec3(155, 155, 155) }, m);
    Light light = { Vec4(0,80,80,1), Vec3(255,255,255), 500 };
    Renderer r;
    r.add({ cube });
    r.add({ plane });
    ID light_id = r.add({ light });
    r.toggle_fps();
    r.set_aa(SSAA);
    r.set_camera_pos(Vec4(0, 0, -10, 1));
    r.launch(2);

    while (true) {
        r.update();
        r.init_controller();
        auto light_r = r.operate_lights(light_id);
        for (const auto& light : light_r) {
            Transform::rotate(light->pos, Vec4(0, 1, 0, 0), pi / 100);
        }
        Sleep(10);
    }
}