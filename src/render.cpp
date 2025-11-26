#include "render.h"
#include "graphics.h"
#include "transform.h"
#include <thread>
#include <windows.h>

Render::Render() {
    Screen::init();
    show = false;
}

void Render::update() {
    screen.update();
    camera.update(screen);
}

void Render::toggle_fps() {
    show = true;
}

void Render::set_aa(AA aa) {
    switch (aa) {
        case SSAA:
            screen.SSAA = true;
            break;
        default:
            break;
    }
}

void Render::set_camera_pos(const Vec4& pos) {
    camera.pos = pos;
}

void Render::launch(int threads) {
    for (int i = 0; i < threads; i++) {
        std::thread(ShadeCycle, std::ref(meshes), std::ref(camera), std::ref(show),
                   std::ref(screen.SSAA), std::ref(screen.width), std::ref(screen.height),
                   std::ref(screen.bias)).detach();
    }
}

[[noreturn]] void Render::ShadeCycle(const std::vector<Mesh>& meshes, const Camera& camera,
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