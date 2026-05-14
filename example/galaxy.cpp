// Galaxy simulation
#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"

struct Planet {
    float orbit_radius;
    float orbit_speed;
    float angle;
    float size;
    Vec4 axis;
    Vec3 color;
    ID mesh_id;
};

float calculate_orbit_speed(float orbit_radius, float central_mass = 100.0f) {
    const float G = 6.67430e-2f;
    return sqrt(G * central_mass * 10 / orbit_radius / orbit_radius / orbit_radius);
}

Planet generate_random_planet(float min_radius, float max_radius) {
    Planet planet;
    planet.orbit_radius = min_radius + (rand() % 100) / 100.0f * (max_radius - min_radius);
    planet.orbit_speed = calculate_orbit_speed(planet.orbit_radius);
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

void generate_planet_system(Renderer& renderer, int planet_count) {
    std::srand((unsigned)std::time(nullptr));

    std::vector<Mesh> sun = {
        Mesh::Cube(Vec4(0, 0, 0, 1), 5, {Vec3(255, 200, 50)},
                  Material{ 0.4f, 0.8f, 0.3f })
    };
    renderer.add_meshes(sun);

    std::vector<Planet> planets;

    float min_radius = 10.0f;
    float max_radius = 200.0f;

    for (int i = 0; i < planet_count; i++) {
        Planet planet = generate_random_planet(min_radius, max_radius);
        float x = planet.orbit_radius * cos(planet.angle);
        float z = planet.orbit_radius * sin(planet.angle);
        float y = rand() % 100 / 8.0f - 5;

        std::vector<Mesh> planet_mesh = {
            Mesh::Cube(Vec4(x, y, z, 1), planet.size, {planet.color},
                      Material{ 0.4f, 0.8f, 0.3f })
        };
        planet.mesh_id = renderer.add_meshes(planet_mesh);
        planet.axis = Vec4(x, y, z, 0).cross3(Vec4(-z, 0, x, 0));
        planets.push_back(planet);
    }

    Light sun_light;
    sun_light.pos = Vec4(0, 80, 0, 1);
    sun_light.color = Vec3(255, 255, 255);
    sun_light.intensity = 1000;

    std::vector<Light> lights = { sun_light };
    renderer.add_lights(lights);

    while (true) {
        renderer.controller();
        renderer.update();

        for (auto& planet : planets) {
            auto planet_meshes = renderer.get_meshes(planet.mesh_id);
            if (!planet_meshes.empty()) {
                Transform::rotate(*planet_meshes[0], planet.axis, planet.orbit_speed);
            }
        }

        platform::sleep_ms(16);
    }
}

int main() {
    Renderer renderer;
    renderer.set_camera_pos(Vec4(0, 50, -100, 1));
    renderer.set_aa(SSAA);
    renderer.launch();
    generate_planet_system(renderer, 500);
    return 0;
}
