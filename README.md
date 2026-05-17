# Console-Renderer

A **single-header C++20** 3D render engine that renders directly to the terminal console via ANSI escape codes. No GPU, no external graphics API — just pure CPU rasterization to `printf`.

## Features

- **Single header** — `#include "render_engine.h"` and you're done
- **Multi-threaded** — tile-based rendering with work stealing
- **Anti-aliasing** — SSAA, FXAA, TAA (with Halton sequence jitter)
- **Blinn-Phong lighting** — ambient, diffuse, specular with multiple lights
- **Triangle rasterization** — scanline fill with perspective-correct interpolation
- **Z-buffer** — per-pixel depth test + hierarchical Z-buffer (Hi-Z) occlusion culling
- **OBJ + MTL loader** — Wavefront `.obj` with material library support
- **FPS camera** — WASD + QE movement, mouse look
- **2D overlay** — Layer2D with 5×5 bitmap font for UI/HUD
- **Rubik's Cube demo** — built-in demo with keyboard controls

## Usage

```cpp
#define RENDER_ENGINE_IMPLEMENTATION
#include "render_engine.h"

int main() {
    Renderer r;
    r.set_camera_pos({ 0, 10, -20, 1 });
    r.set_aa(AA::SSAA);

    auto mesh = Mesh::Cube({ 0, 0, 0, 1 }, 5, { Vec3(200, 200, 200) },
                            Material{ 0.4f, 0.8f, 0.3f });
    r.add_meshes({ mesh });
    r.add_lights({ { { 0, 80, 0, 1 }, { 255, 255, 255 }, 1500 } });

    r.launch();
    while (true) {
        r.controller();
        r.update();
    }
}
```

## Camera Controls

| Key | Action |
|-----|--------|
| W/S | Move forward/backward |
| A/D | Strafe left/right |
| Q/E | Move up/down |
| Mouse | Look around |

## Build

```bash
cmake -B build
cmake --build build
```

Requires C++20 and a terminal that supports ANSI escape codes / virtual terminal sequences.

## Examples

| File | Description |
|------|-------------|
| `example/2048.cpp` | 2048 game rendered to console |
| `example/galaxy.cpp` | 500-planet galaxy simulation with orbital rotation |
| `example/stress_test.cpp` | Performance stress test with hundreds of cubes |
