#include "render.h"
#include "mesh.h"
#include "light.h"
#include "transform.h"
#include "platform.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

// ── Stress test: N×N cubes + orbiting lights ────────────────────────────
#define N 10   // N×N = 100 cubes; try 20 (400), 30 (900)

[[noreturn]] int main() {
    std::srand((unsigned)std::time(nullptr));
    const float spacing = 5.0f, off = (N - 1) * spacing * 0.5f;

    std::vector<Mesh> meshes;
    for (int x = 0; x < N; x++)
        for (int z = 0; z < N; z++)
            meshes.push_back(Mesh::Cube(
                Vec4(x * spacing - off, 0, z * spacing - off, 1), 1.8f,
                {Vec3(rand()%156+100, rand()%156+100, rand()%156+100)},
                Material{0.3f, 0.8f, 0.4f}));

    for (int i = 0; i < N; i++)
        meshes.push_back(Mesh::Cube(
            Vec4((rand()%200-100)*0.2f, 5+rand()%5, (rand()%200-100)*0.2f, 1), 0.8f,
            {Vec3(255, rand()%200+55, 0)},
            Material{0.2f, 0.6f, 1.2f}));

    std::vector<Light> lights = {
        {Vec4(0,80,0,1),  Vec3(255,255,255), 1500},
        // {Vec4(-40,60,40,1),Vec3(200,180,255), 800},
        // {Vec4(40,60,-40,1),Vec3(255,200,180), 800},
        // {Vec4(0,20,60,1),  Vec3(180,255,200), 600},
        // {Vec4(0,100,-20,1),Vec3(255,200,255), 600},
    };

    Renderer r;
    r.set_aa(SSAA);
    ID mesh_id = r.add(meshes);
    ID light_id = r.add(lights);
    r.set_camera_pos(Vec4(0, 20, -40, 1));
    r.launch();

    auto all_meshes = r.operate_meshes(mesh_id);
    auto all_lights = r.operate_lights(light_id);
    std::vector<float> speeds(N, 0);
    for (int i = 0; i < N; i++) speeds[i] = 0.01f + (rand()%100)*0.001f;
    float t = 0;

    while (true) {
        r.update();
        r.controller();
        t += 0.02f;
        for (size_t i = 0; i < all_lights.size(); i++) {
            float a = t + i * 1.256f, r_ = 50 + i * 10.f, h = 20 + sin(t*0.5f+i)*15;
            all_lights[i]->pos = Vec4(cos(a)*r_, h, sin(a)*r_, 1);
        }
        int fstart = N * N;
        for (int i = 0; fstart + i < (int)all_meshes.size(); i++)
            Transform::rotate(*all_meshes[fstart + i], Vec4(1,1,0,0), speeds[i]);
    }
}
