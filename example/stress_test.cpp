// ── Stress test scene ───────────────────────────────────────────────────
// Replace main() in main.cpp with this to run.
//
// Generates N×N cubes + multiple moving lights + 2D overlay FPS.
// Adjust CUBES_PER_SIDE to control mesh count.
//
#define CUBES_PER_SIDE 10   // 10×10 = 100 cubes; try 20 (400), 30 (900)

#include "render.h"
#include "mesh.h"
#include "light.h"
#include "transform.h"
#include "platform.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

[[noreturn]] int main() {
    std::srand((unsigned)std::time(nullptr));

    constexpr int N = CUBES_PER_SIDE;
    const float spacing = 5.0f;
    const float offset = (N - 1) * spacing * 0.5f;

    // Build N×N grid of cubes
    std::vector<Mesh> meshes;
    for (int x = 0; x < N; x++) {
        for (int z = 0; z < N; z++) {
            Vec3 col(
                rand() % 156 + 100,
                rand() % 156 + 100,
                rand() % 156 + 100
            );
            Material mat{ 0.3f, 0.8f, 0.4f };
            meshes.push_back(Mesh::Cube(
                Vec4(x * spacing - offset, 0, z * spacing - offset, 1),
                1.8f, { col }, mat
            ));
        }
    }

    // Extra floating cubes above
    for (int i = 0; i < N; i++) {
        Vec3 col(255, rand() % 200 + 55, 0);
        Material mat{ 0.2f, 0.6f, 1.2f };
        meshes.push_back(Mesh::Cube(
            Vec4((rand() % 200 - 100) * 0.2f,
                 5.0f + rand() % 5,
                 (rand() % 200 - 100) * 0.2f, 1),
            0.8f, { col }, mat
        ));
    }

    // Lights
    std::vector<Light> lights;
    Material m;

    lights.push_back({ Vec4(0, 80, 0, 1), Vec3(255, 255, 255), 1500 });
    lights.push_back({ Vec4(-40, 60, 40, 1), Vec3(200, 180, 255), 800 });
    lights.push_back({ Vec4(40, 60, -40, 1), Vec3(255, 200, 180), 800 });
    lights.push_back({ Vec4(0, 20, 60, 1), Vec3(180, 255, 200), 600 });
    lights.push_back({ Vec4(0, 100, -20, 1), Vec3(255, 200, 255), 600 });

    // Renderer setup
    Renderer r;
    r.set_aa(SSAA);
    r.add(meshes);
    r.add(lights);
    r.set_camera_pos(Vec4(0, 20, 40, 1));
    r.launch();

    // Per-frame rotation offsets for floating cubes
    std::vector<float> rot_speeds;
    for (size_t i = 0; i < N; i++)
        rot_speeds.push_back(0.01f + (rand() % 100) * 0.001f);

    auto all_meshes = r.operate_meshes(ID{0, (int)meshes.size()});
    auto all_lights = r.operate_lights(ID{0, (int)lights.size()});

    float time = 0;

    while (true) {
        r.update();

        // Animate lights — orbit around scene
        time += 0.02f;
        if (!all_lights.empty()) {
            for (size_t i = 0; i < all_lights.size(); i++) {
                float angle = time + i * 1.256f;
                float radius = 50.0f + i * 10.0f;
                float height = 20.0f + sin(time * 0.5f + i) * 15.0f;
                all_lights[i]->pos = Vec4(
                    cos(angle) * radius,
                    height,
                    sin(angle) * radius,
                    1
                );
            }
        }

        // Rotate floating cubes
        int float_start = N * N;
        for (size_t i = 0; i < rot_speeds.size() && float_start + i < all_meshes.size(); i++) {
            Transform::rotate(*all_meshes[float_start + i],
                              Vec4(1, 1, 0, 0), rot_speeds[i]);
        }

        platform::sleep_ms(16);
    }
}
