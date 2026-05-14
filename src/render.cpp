#include "render.h"
#include "graphics.h"
#include "transform.h"
#include "platform.h"
#include <thread>
#include <cstring>
#include <cstdio>
#include <string>

// ── High-precision timing helper (cross-platform, in platform.h) ─────────

// Per-frame timing accumulators (reset every PRINT_INTERVAL frames)
static double t_prepare = 0, t_worker_wait = 0, t_composite = 0, t_total = 0, t_bufalloc = 0;
static double t_aa = 0, t_draw = 0, t_show = 0;
static int frame_count = 0;
static constexpr int PRINT_INTERVAL = 60;

Renderer::Renderer() {
    Screen::init();
    mesh_num = 0;
    light_num = 0;
}

Renderer::~Renderer() {
    shutdown();
}

void Renderer::update() {
    screen.update();
    camera.update_from(screen);
    render_frame();
}

void Renderer::render_frame() {
    Stopwatch frame_sw;

    // Workers copy tiles into screen.buffer — every pixel is overwritten, no clear needed.
    // Only resize when console dimensions change.
    if (static_cast<size_t>(screen.width * screen.height) != screen.buffer.size()) {
        screen.buffer.assign(screen.width * screen.height, Vec3());
        screen.z_buffer.assign(screen.width * screen.height, 0);
    }

    Stopwatch p1_sw;
    prepare_frame();  // frame_meshes now in camera space
    // View-frustum cull: mark invisible meshes (workers skip them)
    frustum_cull();
    // Pre-project all vertices once per frame (avoid per-tile re-projection)
    camera.pre_project_verts(frame_meshes, frame_vert_proj);
    t_prepare += p1_sw.elapsed_ms();

    // ── Adaptive 2D tile partition based on triangle workload ──
    {
        const int w = screen.width;
        const int h = screen.height;
        const int num_wanted = std::max(num_threads * 16, 4);  // fine tiles for work-stealing balance

        // 2D density histogram: 48×48 grid (finer subdivision for better load balance)
        const int GRID_W = 48, GRID_H = 48;
        std::vector<int> hist2d(GRID_W * GRID_H, 0);

        // Accurate screen-space projection (same formula as Camera::load)
        // After prepare_frame(), vertices are in camera space.
        // screen_x = src.x * h / src.z + w/2
        // screen_y = -src.y * h / src.z + h/2
        for (int mi = 0; mi < static_cast<int>(frame_meshes.size()); mi++) {
            if (mi < static_cast<int>(mesh_visible.size()) && !mesh_visible[mi]) continue;
            const auto& mesh = frame_meshes[mi];
            for (size_t ti = 0; ti + 2 < mesh.indices.size(); ti += 3) {
                float sx_sum = 0, sy_sum = 0;
                float min_sx = 1e9f, max_sx = -1e9f, min_sy = 1e9f, max_sy = -1e9f;
                int valid = 0;
                for (int v = 0; v < 3; v++) {
                    const auto& vert = mesh.vertices[mesh.indices[ti + v]];
                    if (vert.z > 0.001f) {
                        const float sx = vert.x * h / vert.z + w * 0.5f;
                        const float sy = -vert.y * h / vert.z + h * 0.5f;
                        sx_sum += sx;
                        sy_sum += sy;
                        if (sx < min_sx) min_sx = sx;
                        if (sx > max_sx) max_sx = sx;
                        if (sy < min_sy) min_sy = sy;
                        if (sy > max_sy) max_sy = sy;
                        valid++;
                    }
                }
                if (valid < 3) continue;
                float cx = sx_sum / 3.0f;
                float cy = sy_sum / 3.0f;
                if (cx < 0 || cx >= w || cy < 0 || cy >= h) continue;
                int col = static_cast<int>(cx * GRID_W / w);
                int row = static_cast<int>(cy * GRID_H / h);
                col = std::clamp(col, 0, GRID_W - 1);
                row = std::clamp(row, 0, GRID_H - 1);
                // Weight by screen-space area (larger triangles = more pixel work)
                const int weight = std::max(1, static_cast<int>((max_sx - min_sx) * (max_sy - min_sy)));
                hist2d[row * GRID_W + col] += weight;
            }
        }

        int total = 0;
        for (int c : hist2d) total += c;
        tiles.clear();

        if (total == 0) {
            tiles.push_back({0, w, 0, h});
        } else {
            // Column-wise sums for x-partitioning
            std::vector<int> col_sum(GRID_W, 0);
            for (int c = 0; c < GRID_W; c++)
                for (int r = 0; r < GRID_H; r++)
                    col_sum[c] += hist2d[r * GRID_W + c];

            const int target_per_tile = std::max(1, total / num_wanted);

            // ── Step 1: Greedy column splits (x direction) ──
            struct ColSplit { int x0, x1; };
            std::vector<ColSplit> col_splits;
            int acc = 0, cx0 = 0;
            for (int c = 0; c < GRID_W && static_cast<int>(col_splits.size()) < num_wanted - 1; c++) {
                acc += col_sum[c];
                if (acc >= target_per_tile) {
                    int x1 = (c + 1) * w / GRID_W;
                    if (x1 > cx0) {
                        col_splits.push_back({cx0, x1});
                        cx0 = x1;
                        acc = 0;
                    }
                }
            }
            if (w > cx0) col_splits.push_back({cx0, w});

            // ── Step 2: For each column, split into rows (y direction) ──
            for (const auto& col : col_splits) {
                int col_gx0 = col.x0 * GRID_W / w;
                int col_gx1 = (col.x1 * GRID_W + w - 1) / w;
                col_gx1 = std::min(col_gx1, GRID_W);

                // Row histogram for this column
                std::vector<int> row_hist(GRID_H, 0);
                for (int r = 0; r < GRID_H; r++)
                    for (int c = col_gx0; c < col_gx1; c++)
                        row_hist[r] += hist2d[r * GRID_W + c];

                int col_total = 0;
                for (int r = 0; r < GRID_H; r++) col_total += row_hist[r];

                if (col_total == 0) {
                    tiles.push_back({col.x0, col.x1, 0, h});
                    continue;
                }

                // Dynamic row cap: proportional to desired total tiles ÷ number of columns
                const int max_rows_per_col = std::max(1, num_wanted / std::max(1, static_cast<int>(col_splits.size())));
                int num_rows = std::max(1, std::min(col_total / target_per_tile, max_rows_per_col));
                int row_target = std::max(1, col_total / num_rows);

                acc = 0;
                int ry0 = 0;
                int rows_done = 0;
                for (int r = 0; r < GRID_H && rows_done < num_rows - 1; r++) {
                    acc += row_hist[r];
                    if (acc >= row_target) {
                        int y1 = (r + 1) * h / GRID_H;
                        if (y1 > ry0) {
                            tiles.push_back({col.x0, col.x1, ry0, y1});
                            ry0 = y1;
                            acc = 0;
                            rows_done++;
                        }
                    }
                }
                if (h > ry0) tiles.push_back({col.x0, col.x1, ry0, h});
            }
        }
    }

    if (aa_mode == TAA) {
        Vec2 j = Screen::halton_sequence(screen.get_taa_frame_count() + 1);
        constexpr float SCALE = 1000000.0f;
        camera.set_jitter(j.x / SCALE, j.y / SCALE);
    } else {
        camera.set_jitter(0, 0);
    }

    Stopwatch ww_sw;
    tile_index.store(0, std::memory_order_relaxed);
    workers_done.store(0, std::memory_order_release);
    current_frame.fetch_add(1, std::memory_order_acq_rel);
    cv.notify_all();

    // Main thread also processes tiles (instead of busy-waiting, improving CPU utilization)
    {
        TileScreen ts_main(0, 0, 0, 0, 0, 0);
        int ti;
        while ((ti = tile_index.fetch_add(1, std::memory_order_relaxed)) < static_cast<int>(tiles.size()))
            render_tile(tiles[ti], ts_main);
    }
    // Wait for remaining workers (brief — main thread already processed a share)
    while (workers_done.load(std::memory_order_acquire) < num_threads)
        std::this_thread::yield();
    t_worker_wait += ww_sw.elapsed_ms();

    // Composite 2D overlay layers on top of 3D scene
    composite_layers();

    Stopwatch cmp_sw;
    composite_frame();
    t_composite += cmp_sw.elapsed_ms();

    double frame_ms = frame_sw.elapsed_ms();
    screen.calculate_fps(frame_ms);
    t_total += frame_ms;

    // Print profile every PRINT_INTERVAL frames
    frame_count++;
    if (frame_count >= PRINT_INTERVAL) {
        char buf[320];
        const int n = frame_count;
        std::snprintf(buf, sizeof(buf),
            "[PROFILE %4dx%-3d last %4d frames]  "
            "alloc=%5.2f  prep=%5.2f  "
            "worker=%6.2f  cmp(AA=%5.2f drw=%5.2f shw=%6.2f)=%6.2f  "
            "total=%6.2fms  FPS=%.0f\n",
            screen.width, screen.height, n,
            t_bufalloc / n, t_prepare / n,
            t_worker_wait / n, t_aa / n, t_draw / n, t_show / n, t_composite / n,
            t_total / n, 1000.0 / (t_total / n));
        FILE* fp = fopen("profile.log", "a");
        if (fp) {
            if (ftell(fp) == 0) fputs("===== Profile Log =====\n", fp);
            fputs(buf, fp);
            fclose(fp);
        }
        t_bufalloc = t_prepare = t_worker_wait = t_composite = t_total = 0;
        t_aa = t_draw = t_show = 0;
        frame_count = 0;
    }
}

void Renderer::frustum_cull() {
    const size_t n = frame_meshes.size();
    mesh_visible.resize(n, true);
    if (n == 0) return;

    // Camera frustum in camera space:
    //   near z = n_near (1), far z = n_far (100)
    //   x = ± aspect * z / (2 * n_near)
    //   y = ± z / (2 * n_near)   (height = 1)
    const float cam_n = 1.0f;
    const float cam_f = 100.0f;
    const float aspect = camera.width;   // screen.width / screen.height
    const float inv_2n = 1.0f / (2.0f * cam_n);

    for (size_t i = 0; i < n; i++) {
        const auto& mesh = frame_meshes[i];
        const float cx = mesh.center.x;
        const float cy = mesh.center.y;
        const float cz = mesh.center.z;
        const float r = mesh.bounding_radius;
        if (r <= 0) continue;  // no bounding volume, always visible

        // Reject behind near plane
        if (cz + r < cam_n) { mesh_visible[i] = false; continue; }
        // Reject beyond far plane
        if (cz - r > cam_f) { mesh_visible[i] = false; continue; }

        // Reject outside horizontal frustum (conservative: test at closest z)
        float z_test = std::max(cz - r, cam_n);  // sphere's front toward camera
        float half_w = aspect * z_test * inv_2n;
        if (cx - r > half_w || cx + r < -half_w) { mesh_visible[i] = false; continue; }

        // Reject outside vertical frustum
        float half_h = z_test * inv_2n;
        if (cy - r > half_h || cy + r < -half_h) { mesh_visible[i] = false; continue; }
    }
}

void Renderer::prepare_frame() {
    // Transform meshes to camera space (in-place on frame copies)
    // Avoid reallocation when mesh count is stable
    if (frame_meshes.size() != meshes.size())
        frame_meshes = meshes;
    else
        for (size_t i = 0; i < meshes.size(); i++)
            frame_meshes[i] = meshes[i];

    // Build combined view matrix: M = Rx(-a_y) * Ry(-a_x) * T(-camera.pos)
    // (eliminates per-vertex Rodrigues rotation overhead)
    const float cos_ax = std::cos(camera.a_x);
    const float sin_ax = std::sin(camera.a_x);
    const float cos_ay = std::cos(camera.a_y);
    const float sin_ay = std::sin(camera.a_y);
    Mat4 Ry(Vec4(cos_ax, 0, -sin_ax, 0),
            Vec4(0, 1, 0, 0),
            Vec4(sin_ax, 0, cos_ax, 0),
            Vec4(0, 0, 0, 1));
    Mat4 Rx(Vec4(1, 0, 0, 0),
            Vec4(0, cos_ay, sin_ay, 0),
            Vec4(0, -sin_ay, cos_ay, 0),
            Vec4(0, 0, 0, 1));
    Mat4 T(Vec4(1, 0, 0, -camera.pos.x),
           Vec4(0, 1, 0, -camera.pos.y),
           Vec4(0, 0, 1, -camera.pos.z),
           Vec4(0, 0, 0, 1));
    const Mat4 view_matrix = Rx * Ry * T;

    for (auto& mesh : frame_meshes)
        for (auto& v : mesh.vertices)
            v = view_matrix * v;

    // Transform lights to camera space
    frame_lights = lights;
    for (auto& light : frame_lights)
        light.pos = view_matrix * light.pos;
}

void Renderer::composite_layers() {
    if (overlay.width() != screen.width || overlay.height() != screen.height) {
        overlay = Layer2D(screen.width, screen.height, 10);
    }
    overlay.clear();

    // FPS overlay
    overlay.draw_text(0.01f, 0.01f, Screen::get_fps_display(), Vec3(200, 200, 200), Vec3(0, 0, 0), 0.025f);

    overlay.composite_to(screen.buffer, screen.width, screen.height);
}

void Renderer::composite_frame() {
    Stopwatch sw_aa;
    // SSAA is handled per-worker (2x render → downsample → 1x write), no separate pass needed
    switch (aa_mode) {
        case FXAA: screen.apply_fxaa(); break;
        case TAA:  screen.apply_taa();  break;
        default: break;
    }
    t_aa += sw_aa.elapsed_ms();

    Stopwatch sw_draw;
    screen.draw();
    t_draw += sw_draw.elapsed_ms();

    Stopwatch sw_show;
    screen.show();
    t_show += sw_show.elapsed_ms();
}

void Renderer::render_tile(const Tile& tile, TileScreen& ts) {
    const int tw = tile.x_end - tile.x_start;
    const int th = tile.y_end - tile.y_start;

    if (screen.SSAA) {
        const int tw2 = tw * 2, th2 = th * 2;
        ts.resize_if_needed(screen.width * 2, screen.height * 2,
                            tile.x_start * 2, tile.y_start * 2, tw2, th2);
        Tile clip2{ tile.x_start * 2, tile.x_end * 2,
                    tile.y_start * 2, tile.y_end * 2 };
        const Vec4* proj_ptr = frame_vert_proj.data();
        for (int mi = 0; mi < static_cast<int>(frame_meshes.size()); mi++) {
            if (mi < static_cast<int>(mesh_visible.size()) && !mesh_visible[mi]) {
                proj_ptr += frame_meshes[mi].vertices.size();
                continue;
            }
            camera.load(ts, frame_meshes[mi], frame_lights, &clip2, proj_ptr);
            proj_ptr += frame_meshes[mi].vertices.size();
        }

        for (int y = 0; y < th; y++)
            for (int x = 0; x < tw; x++) {
                const size_t s = static_cast<size_t>(x * 2) + static_cast<size_t>(y * 2) * static_cast<size_t>(tw2);
                ts.buffer[static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(tw)] = Vec3(
                    (ts.buffer[s].x + ts.buffer[s + 1].x + ts.buffer[s + tw2].x + ts.buffer[s + tw2 + 1].x) / 4,
                    (ts.buffer[s].y + ts.buffer[s + 1].y + ts.buffer[s + tw2].y + ts.buffer[s + tw2 + 1].y) / 4,
                    (ts.buffer[s].z + ts.buffer[s + 1].z + ts.buffer[s + tw2].z + ts.buffer[s + tw2 + 1].z) / 4);
            }

        const size_t row1 = static_cast<size_t>(screen.width);
        for (int y = 0; y < th; y++)
            std::memcpy(&screen.buffer[static_cast<size_t>(tile.x_start) + static_cast<size_t>(tile.y_start + y) * row1],
                        &ts.buffer[static_cast<size_t>(y) * static_cast<size_t>(tw)],
                        static_cast<size_t>(tw) * sizeof(Vec3));
    } else {
        ts.resize_if_needed(screen.width, screen.height,
                            tile.x_start, tile.y_start, tw, th);
        const Vec4* proj_ptr = frame_vert_proj.data();
        for (int mi = 0; mi < static_cast<int>(frame_meshes.size()); mi++) {
            if (mi < static_cast<int>(mesh_visible.size()) && !mesh_visible[mi]) {
                proj_ptr += frame_meshes[mi].vertices.size();
                continue;
            }
            camera.load(ts, frame_meshes[mi], frame_lights, &tile, proj_ptr);
            proj_ptr += frame_meshes[mi].vertices.size();
        }

        const size_t row_width = static_cast<size_t>(screen.width);
        for (int y = 0; y < th; y++)
            std::memcpy(&screen.buffer[static_cast<size_t>(tile.x_start) + static_cast<size_t>(tile.y_start + y) * row_width],
                        &ts.buffer[static_cast<size_t>(y) * static_cast<size_t>(tw)],
                        static_cast<size_t>(tw) * sizeof(Vec3));
    }
}

void Renderer::worker_loop(const int thread_id) {
    int my_frame = 0;

    while (true) {
        // Wait for frame signal or shutdown
        std::unique_lock lock(mtx);
        cv.wait(lock, [&] {
            return !running || current_frame.load(std::memory_order_acquire) != my_frame;
        });
        if (!running) return;
        lock.unlock();

        // Dynamic work distribution — reuse TileScreen buffer across tiles to avoid page faults
        TileScreen ts(0, 0, 0, 0, 0, 0);
        int ti;
        while ((ti = tile_index.fetch_add(1, std::memory_order_relaxed)) < static_cast<int>(tiles.size()))
            render_tile(tiles[ti], ts);

        // Signal completion
        my_frame = current_frame.load(std::memory_order_acquire);
        workers_done.fetch_add(1, std::memory_order_release);
    }
}

void Renderer::toggle_fps() {
    screen.show_fps = !screen.show_fps;
}

void Renderer::set_aa(const AA aa) {
    aa_mode = aa;
    screen.SSAA = (aa == SSAA);
}

void Renderer::set_camera_pos(const Vec4& pos) {
    camera.pos = pos;
}

// Count available hardware threads (cross-platform)
static int hardware_thread_count() {
    return std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
}

void Renderer::launch() {
    if (running) return;

    num_threads = hardware_thread_count();
    running = true;

    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(&Renderer::worker_loop, this, i);
    }
}

void Renderer::shutdown() {
    if (!running) return;
    running = false;
    cv.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers.clear();
}

ID Renderer::add(const std::vector<Mesh>& _meshes) {
    const int begin = mesh_num;
    for (const auto& mesh : _meshes) {
        meshes.emplace_back(mesh);
        ++mesh_num;
    }
    return ID{ begin, mesh_num };
}

ID Renderer::add(const std::vector<Light>& _lights) {
    const int begin = light_num;
    for (const auto& light : _lights) {
        lights.emplace_back(light);
        ++light_num;
    }
    return ID{ begin, light_num };
}

std::vector<Mesh*> Renderer::operate_meshes(const ID id) {
    std::vector<Mesh*> mesh;
    for (int i = id.begin; i < id.end ; i++) {
        mesh.emplace_back(&meshes[i]);
    }
    return mesh;
}

std::vector<Light*> Renderer::operate_lights(const ID id) {
    std::vector<Light*> light;
    for (int i = id.begin; i < id.end ; i++) {
        light.emplace_back(&lights[i]);
    }
    return light;
}

void Renderer::controller() {
    camera.controller();
}

Screen* Renderer::get_screen() {
    return &screen;
}

Camera* Renderer::get_camera() {
    return &camera;
}
