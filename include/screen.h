
#ifndef PROJECT_SCREEN_H
#define PROJECT_SCREEN_H
#pragma once
#include "math_utils.h"
#include <string>
#include <vector>
#include <ctime>

class Screen {
public:
    Screen();
    static void init();
    void update();
    void set_pixel(int x, int y, const Vec3& color, float z);
    void clear();
    void draw();
    static void calculate_fps();
    void show() const;

    std::string output_buf;
    std::vector<Vec3> buffer;
    std::vector<float> z_buffer;
    int width, height;
    bool bias, SSAA, show_fps;

private:
    static clock_t start_t, end_t;
    static int counter_t, fps;
};
#endif //PROJECT_SCREEN_H