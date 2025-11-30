#include "render.h"
#include "graphics.h"
#include "transform.h"
#include <thread>
#include <windows.h>

Renderer::Renderer() {
    Screen::init();
    mesh_num = 0;
}

void Renderer::update() {
    screen.update();
    camera.update_from(screen);
}

void Renderer::toggle_fps() {
    screen.show_fps = !screen.show_fps;
}

void Renderer::set_aa(AA aa) {
    switch (aa) {
        case SSAA:
            screen.SSAA = true;
            break;
        default:
            break;
    }
}

void Renderer::set_camera_pos(const Vec4& pos) {
    camera.pos = pos;
}

void Renderer::launch(const int threads) {
    for (int i = 0; i < threads; i++) {
        std::thread(&Renderer::ShadeCycle, this).detach();
    }
}

ID Renderer::add_meshes(const std::vector<Mesh>& _meshes) {
    const int begin = mesh_num;
    for (const auto& mesh : _meshes) {
        meshes.emplace_back(mesh);
        ++mesh_num;
    }
    return ID(begin, mesh_num);
}

std::vector<Mesh*> Renderer::operate_meshes(const ID id) {
    std::vector<Mesh*> mesh;
    for (int i = 0; i < id.end - id.begin; i++) {
        mesh.emplace_back(&meshes[i]);
    }
    return mesh;
}

void Renderer::init_controller() {
    camera.controller();
}


[[noreturn]] void Renderer::ShadeCycle() const {
    Screen new_screen;

    while (true) {
        new_screen.show_fps = screen.show_fps;
            new_screen.SSAA = screen.SSAA;
        if (new_screen.width != screen.width || new_screen.height != screen.height || new_screen.bias != screen.bias) {
            new_screen.width = screen.width;
            new_screen.height = screen.height;
            new_screen.bias = screen.bias;\

        }
        new_screen.clear();
        for (auto mesh : meshes) {
            Transform::translate(mesh, camera.pos * (-1));
            Transform::rotate(mesh, Vec4(0, 1, 0, 0), -camera.a_x);
            Transform::rotate(mesh, Vec4(1, 0, 0, 0), -camera.a_y);

            camera.load(new_screen, mesh);
        }
        Screen::calculate_fps();
        new_screen.draw();
        new_screen.show();
    }
}
