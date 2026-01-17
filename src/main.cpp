//#include "render.h"
//#include "rubik_cube.h"
//
//[[noreturn]] int main() {
//    //魔方场景
//    //auto cube = RubikCube(2);
//    //auto light = Light{ Vec4(0,6,15,1), Vec3(255,255,255), 5 };
//    //Renderer r;
//
//    //r.toggle_fps();
//    //r.set_aa(SSAA);
//    //for (int i = 0; i < 10; i++) {
//    //    for (int j = 0; j < 10; j++) {
//    //        for (const auto& mesh : r.operate_meshes(r.add(cube.meshes))) {
//    //            Transform::rotate(*mesh, Vec4(1, 0, 0, 0), pi / 10 * i);
//    //            Transform::rotate(*mesh, Vec4(0, 0, 1, 0), pi / 10 * j);
//    //            Transform::translate(*mesh, Vec4(i * 5 - 25, 0, j * 5 - 25, 0));
//    //        }
//    //    }
//    //}
//    //// const ID rubik_cube_id = r.add(cube.meshes);
//    //const ID light_id = r.add({light});
//    //r.set_camera_pos(Vec4(0, 0, -10, 1));
//    //r.launch(2);
//
//    //const auto rubik_indices = cube.block_indices;
//    // const auto rubik_mesh = r.operate_meshes(rubik_cube_id);
//    //while (true) {
//    //    r.update();
//    //    for (const auto& _light : r.operate_lights(light_id)) {
//    //        Transform::rotate(_light->pos, Vec4(0, 1, 0, 0), pi / 100);
//    //    }
//    //    // RubikCube::control(rubik_indices, rubik_mesh);
//    //    r.controller();
//    //    Sleep(10);
//    //}
//
//    //立方体场景
//   /* Material m = { 0.3, 1.6, 0.5 };
//    Mesh cube = Mesh::Cube(Vec4(0, 0, 0, 1), 4, { Vec3(164, 227, 240) }, m);
//    Mesh plane = Mesh::Plane(Vec4(0, -5, 0, 1), 100, { Vec3(155, 155, 155) }, m);
//    Light light = { Vec4(0,80,80,1), Vec3(255,255,255), 500 };
//    Renderer r;
//    r.add({ cube });
//    r.add({ plane });
//    ID light_id = r.add({ light });
//    r.toggle_fps();
//    r.set_aa(SSAA);
//    r.set_camera_pos(Vec4(0, 0, -10, 1));
//    r.launch(2);
//
//    while (true) {
//        r.update();
//        r.controller();
//        auto light_r = r.operate_lights(light_id);
//        for (const auto& light : light_r) {
//            Transform::rotate(light->pos, Vec4(0, 1, 0, 0), pi / 100);
//        }
//        Sleep(10);
//    }*/
//
//        //环绕场景
//        //// 初始化渲染器
//        //Renderer renderer;
//
//        //// 添加光源
//        //std::vector<Light> lights;
//        //lights.push_back({ Vec4(0, 80, 0, 1), Vec3(255, 255, 255), 400.0f });  // 顶部白光
//        //lights.push_back({ Vec4(-50, 60, 0, 1), Vec3(200, 200, 255), 1000.0f });  // 左侧偏蓝光
//        //lights.push_back({ Vec4(50, 60, 0, 1), Vec3(255, 200, 200), 1000.0f });   // 右侧偏红光
//
//        //ID light_id = renderer.add(lights);
//
//        //// 创建材质
//        //Material metal_material = { 0.2f, 0.5f, 1.5f };      // 金属材质
//        //Material plastic_material = { 0.4f, 0.8f, 0.3f };    // 塑料材质
//        //Material wood_material = { 0.6f, 1.0f, 0.1f };       // 木质材质
//        //Material shiny_material = { 0.1f, 0.3f, 2.0f };      // 反光材质
//
//        //// 添加地面
//        //std::vector<Mesh> ground_meshes;
//        //for (int i = -5; i <= 5; i++) {
//        //    for (int j = -5; j <= 5; j++) {
//        //        Vec3 color = (i + j) % 2 == 0 ? Vec3(100, 100, 100) : Vec3(150, 150, 150);
//        //        ground_meshes.push_back(Mesh::Plane(Vec4(i * 4, -3, j * 4, 1), 2.0f,
//        //            { color }, plastic_material));
//        //    }
//        //}
//        //ID ground_id = renderer.add(ground_meshes);
//
//        //// 添加各种几何体
//        //std::vector<Mesh> scene_meshes;
//
//        //// 中心金字塔
//        //scene_meshes.push_back(Mesh::Cube(Vec4(0, 0, 0, 1), 1.0f,
//        //    { Vec3(255, 100, 100) }, metal_material));
//
//        //// 周围柱子
//        //for (int i = 0; i < 8; i++) {
//        //    float angle = i * 3.14159f / 4;
//        //    float x = 5 * cos(angle);
//        //    float z = 5 * sin(angle);
//
//        //    Vec3 color;
//        //    if (i % 3 == 0) color = Vec3(100, 255, 100);
//        //    else if (i % 3 == 1) color = Vec3(100, 100, 255);
//        //    else color = Vec3(255, 255, 100);
//
//        //    scene_meshes.push_back(Mesh::Cube(Vec4(x, 0, z, 1), 0.5f,
//        //        { color }, plastic_material));
//        //}
//
//        //// 浮动的立方体
//        //for (int i = 0; i < 4; i++) {
//        //    float height = 2 + sin(i * 1.57f) * 1.5f;
//        //    float x = (i - 1.5f) * 3;
//
//        //    scene_meshes.push_back(Mesh::Cube(Vec4(x, height, -2, 1), 0.4f,
//        //        { Vec3(200, 150, 255) }, shiny_material));
//        //}
//
//        //ID scene_id = renderer.add(scene_meshes);
//
//        //// 设置相机初始位置
//        //renderer.set_camera_pos(Vec4(0, 0, 10, 1));
//
//        //// 启用SSAA抗锯齿
//        //renderer.set_aa(SSAA);
//
//        //// 显示FPS
//        //renderer.toggle_fps();
//
//        //// 启动渲染线程
//        //renderer.launch(2);  // 使用2个渲染线程
//
//        //bool running = true;
//        //float time = 0.0f;
//
//        //while (running) {
//        //    // 处理退出
//        //    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
//        //        running = false;
//        //    }
//
//        //    // 控制相机
//        //    renderer.controller();
//
//        //    // 更新渲染器（处理窗口大小变化等）
//        //    renderer.update();
//
//        //    // 获取场景物体指针进行动态修改
//        //    auto scene_objects = renderer.operate_meshes(scene_id);
//
//        //    // 让浮动的立方体上下移动
//        //    time += 0.01f;
//        //    for (int i = scene_meshes.size() - 4; i < scene_meshes.size(); i++) {
//        //        if (i < scene_objects.size()) {
//        //            Mesh* cube = scene_objects[i];
//
//        //            // 保存原始位置（只修改Y坐标）
//        //            float original_x = cube->center.x;
//        //            float original_z = cube->center.z;
//        //            float offset = i - (scene_meshes.size() - 4);
//        //            float new_y = 2 + sin(time + offset) * 1.5f;
//
//        //            // 重置到原始位置再设置新高度
//        //            cube->center = Vec4(original_x, new_y, original_z, 1);
//
//        //            // 同时旋转立方体
//        //            Transform::rotate(*cube, Vec4(0, 1, 0, 0), 0.01f);
//        //            Transform::rotate(*cube, Vec4(1, 0, 0, 0), 0.005f);
//        //        }
//        //    }
//
//        //    // 让中心金字塔旋转
//        //    if (scene_objects.size() > 0) {
//        //        Transform::rotate(*scene_objects[0], Vec4(0, 1, 0, 0), 0.02f);
//        //    }
//
//        //    // 让柱子缓慢旋转
//        //    for (int i = 1; i <= 8 && i < scene_objects.size(); i++) {
//        //        float speed = 0.01f * (i % 3 + 1);
//        //        Transform::rotate(*scene_objects[i], Vec4(0, 1, 0, 0), speed);
//        //    }
//
//        //    // 简单延迟，控制更新频率
//        //    Sleep(10);
//        //}
//
//}
#include "render.h"
#include "mesh.h"
#include "light.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

// 星球结构体，包含轨道参数和物理属性
struct Planet {
    float orbit_radius;      // 轨道半径
    float orbit_speed;       // 轨道速度
    float angle;            // 当前角度
    float size;             // 星球大小
    Vec4 axis;
    Vec3 color;             // 星球颜色
    ID mesh_id;             // 对应的网格ID
};

// 开普勒第三定律计算轨道速度
float calculate_orbit_speed(float orbit_radius, float central_mass = 100.0f) {
    // 开普勒第三定律: T² ∝ a³
    // 简化: 速度与 √(中心质量/半径) 成反比
    const float G = 6.67430e-2f;  // 简化后的引力常数
    return sqrt(G * central_mass * 50 / orbit_radius / orbit_radius);
}

// 生成随机星球
Planet generate_random_planet(float min_radius, float max_radius) {
    Planet planet;

    planet.orbit_radius = min_radius + (rand() % 100) / 100.0f * (max_radius - min_radius);

    // 使用开普勒第三定律计算轨道速度
    planet.orbit_speed = calculate_orbit_speed(planet.orbit_radius);

    // 随机初始角度
    planet.angle = (rand() % 360) * 3.14159f / 180.0f;

    planet.size = 1.4;

    if(planet.orbit_radius < 0.25 * max_radius + 0.75 * min_radius)
        planet.color = Vec3(255, 255 * (planet.orbit_radius - min_radius) / (max_radius - min_radius) * 4, 0);
    else if (planet.orbit_radius < 0.5 * max_radius + 0.5 * min_radius)
        planet.color = Vec3(255 * (0.5 * max_radius + 0.5 * min_radius - planet.orbit_radius) / (max_radius - min_radius) * 4, 255, 0);
    else if (planet.orbit_radius < 0.75 * max_radius + 0.25 * min_radius)
        planet.color = Vec3(0, 255, 255 * (planet.orbit_radius - 0.5 * max_radius + 0.5 * min_radius) / (max_radius - min_radius) * 4);
    else 
        planet.color = Vec3(0, 255 * (max_radius - planet.orbit_radius) / (max_radius - min_radius) * 4, 255);



    return planet;
}

// 生成测试场景
void generate_planet_system(Renderer& renderer, int planet_count) {
    std::srand(std::time(nullptr));

    // 清空现有物体
    // 注意：实际实现中可能需要清空功能，这里假设每次重新生成

    // 1. 生成中心恒星
    std::vector<Mesh> sun = {
        Mesh::Cube(Vec4(0, 0, 0, 1), 5, {Vec3(255, 200, 50)},
                  Material{ 0.4f, 0.8f, 0.3f })
    };
    ID sun_id = renderer.add(sun);

    // 2. 生成行星
    std::vector<Planet> planets;
    std::vector<Mesh> all_planets;

    float min_radius = 9.5f;
    float max_radius = 200.0f;

    for (int i = 0; i < planet_count; i++) {
        Planet planet = generate_random_planet(min_radius, max_radius);

        // 初始位置
        float x = planet.orbit_radius * cos(planet.angle);
        float z = planet.orbit_radius * sin(planet.angle);
        float y = rand() % 100 / 8. - 5;

        // 创建星球立方体
        std::vector<Mesh> planet_mesh = {
            Mesh::Cube(Vec4(x, y, z, 1), planet.size, {planet.color},
                      Material{ 0.4f, 0.8f, 0.3f })
        };
        planet.mesh_id = renderer.add(planet_mesh);
        planet.axis = Vec4(x, y, z, 0).cross3(Vec4(-z, 0, x, 0));
        planets.push_back(planet);
    }

    // 3. 添加光源（恒星作为光源）
    Light sun_light;
    sun_light.pos = Vec4(0, 80, 0, 1);  // 在恒星上方
    sun_light.color = Vec3(255, 255, 255);
    sun_light.intensity = 1000;

    std::vector<Light> lights = { sun_light };
    renderer.add(lights);


    // 主循环更新行星位置
    while (true) {
        renderer.controller();
        renderer.update();

        // 更新所有行星的位置
        for (auto& planet : planets) {
            auto planet_meshes = renderer.operate_meshes(planet.mesh_id);
            if (!planet_meshes.empty()) {
                Transform::rotate(*planet_meshes[0], planet.axis, planet.orbit_speed * 0.02f);
            }
        }

        Sleep(16);  // 约60FPS
    }
}

int main() {
    // 初始化渲染器
    Renderer renderer;

    // 设置相机位置（俯视整个星系）
    renderer.set_camera_pos(Vec4(0, 50, -100, 1));
    renderer.set_aa(SSAA);
    // 启动渲染线程
    renderer.launch(2);

    // 生成星系场景
    generate_planet_system(renderer, 500);

    return 0;
}