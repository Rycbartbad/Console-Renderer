
#ifndef PROJECT_RENDER_H
#define PROJECT_RENDER_H
#include "math_utils.h"
#include "mesh.h"
#include "camera.h"
#include "screen.h"
#include "transform.h"
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

enum AA { SSAA, NOAA, FXAA, TAA };

struct ID {
    int begin;
    int end;
};

class Renderer{
public:
    Renderer();
    ~Renderer();
    void update();
    void toggle_fps();
    void set_aa(AA);
    void set_camera_pos(const Vec4&);
    void controller();
    void launch();
    void shutdown();
    Screen* get_screen();
    Camera* get_camera();
    ID add(const std::vector<Mesh>&);
    ID add(const std::vector<Light>&);
    std::vector<Mesh*> operate_meshes(ID);
    std::vector<Light*> operate_lights(ID);

private:
    void prepare_frame();
    void composite_frame();
    void render_frame();
    void render_tile(const Tile& tile, TileScreen& ts);
    void worker_loop(int thread_id);

    Camera camera;
    Screen screen;
    std::vector<Mesh> meshes;
    std::vector<Light> lights;
    int mesh_num;
    int light_num;

    // Per-frame transformed copies (prepared single-threaded, consumed by tile workers)
    std::vector<Mesh> frame_meshes;
    std::vector<Light> frame_lights;

    // Per-frame vertex projection cache (avoids re-projection per tile per mesh)
    std::vector<Vec4> frame_vert_proj;

    // Thread synchronization
    std::vector<std::thread> workers;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> current_frame{0};
    std::atomic<int> workers_done{0};
    bool running{false};
    int num_threads{1};
    std::vector<Tile> tiles;
    std::atomic<int> tile_index{0};  // work-stealing counter
    AA aa_mode{NOAA};
};
#endif //PROJECT_RENDER_H
