#include "render.h"
#include "graphics.h"
#include "transform.h"
#include <thread>
#include <windows.h>

Renderer::Renderer() {
    Screen::init();
    show = false;
    mesh_num = 0;
}

void Renderer::update() {
    screen.update();
    camera.update(screen);
}

void Renderer::toggle_fps() {
    show = true;
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
        std::thread(ShadeCycle, std::ref(meshes), std::ref(camera), std::ref(show),
                   std::ref(screen.SSAA), std::ref(screen.width), std::ref(screen.height),
                   std::ref(screen.bias)).detach();
    }
}

ID Renderer::add_meshes(const std::vector<Mesh>& _meshes) {
    const int begin = mesh_num;
    for (auto mesh : _meshes) {
        meshes.emplace_back(std::move(mesh));
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

[[noreturn]] void Renderer::ShadeCycle(const std::vector<Mesh>& meshes, const Camera& camera,
                                   const bool& show_fps, const bool& SSAA, const int& width,
                                   const int& height, const bool& bias) {
    Screen screen;

    while (true) {
        screen.show_fps = show_fps;
        screen.SSAA = SSAA;
        if (screen.width != width || screen.height != height || screen.bias != bias) {
            screen.width = width;
            screen.height = height;
            screen.bias = bias;
        }
        screen.clear();
        for (auto mesh : meshes) {
            Transform::translate(mesh, camera.pos * (-1));
            Transform::rotate(mesh, Vec4(0, 1, 0, 0), camera.a_x);
            Transform::rotate(mesh, Vec4(1, 0, 0, 0), -camera.a_y);

            camera.load(screen, mesh);
        }
        Screen::calculate_fps();
        screen.draw();
        screen.show();
    }
}