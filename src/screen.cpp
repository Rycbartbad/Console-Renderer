#include "screen.h"
#include <iostream>
#include <windows.h>

clock_t Screen::start_t = 0;
clock_t Screen::end_t = 0;
int Screen::counter_t = 0;
int Screen::fps = 0;

Screen::Screen() {
    width = height = 0;
    fps = 0;
    bias = false;
    SSAA = false;
    show_fps = false;
}

void Screen::init() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    printf("\033[?25l");
    start_t = clock();
    counter_t = 0;
}

void Screen::update() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    csbi.dwSize.X = csbi.dwMaximumWindowSize.X;
    csbi.dwSize.Y = csbi.dwMaximumWindowSize.Y;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);
    width = csbi.dwSize.X / 2;
    height = csbi.dwSize.Y;
    bias = csbi.dwSize.X % 2;
    if (SSAA) {
        width *= 2;
        height *= 2;
    }
}

void Screen::set_pixel(int x, int y, const Vec3& color, float z) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    const size_t index = x + y * width;
    if (z_buffer[index] != 0 && z >= z_buffer[index]) return;
    z_buffer[index] = z;
    buffer[index] = color;
}

void Screen::clear() {
    buffer.assign(width * height, Vec3());
    z_buffer.assign(width * height, 0);
}

void Screen::draw() {
    int x = 0;
    if (show_fps) {
        x = ("FPS:" + std::to_string(fps)).size() / 2;
        output_buf = "\033[3J\033[HFPS:" + std::to_string(fps);
        if (std::to_string(fps).size() % 2 != 0) {
            ++x;
            output_buf += " ";
        }
    } else {
        output_buf = "\033[3J\033[H";
    }
    
    if (SSAA) {
        for (int y = 0; y < height; y += 2) {
            for (x *= 2; x < width; x += 2) {
                output_buf += "\033[48;2;" +
                    std::to_string((buffer[x + y * width].x + buffer[x + 1 + y * width].x + 
                                  buffer[x + y * width + width].x + buffer[x + 1 + y * width + width].x) / 4) + ';' +
                    std::to_string((buffer[x + y * width].y + buffer[x + 1 + y * width].y + 
                                  buffer[x + y * width + width].y + buffer[x + 1 + y * width + width].y) / 4) + ';' +
                    std::to_string((buffer[x + y * width].z + buffer[x + 1 + y * width].z + 
                                  buffer[x + y * width + width].z + buffer[x + 1 + y * width + width].z) / 4) + "m  ";
            }
            x = 0;
            if (bias)
                output_buf += "\033[m ";
        }
    } else {
        for (int y = 0; y < height; y++) {
            for (; x < width; x++) {
                if (z_buffer[x + y * width] != 0) {
                    output_buf += "\033[48;2;" + std::to_string(buffer[x + y * width].x) + ';' + 
                                 std::to_string(buffer[x + y * width].y) + ';' + 
                                 std::to_string(buffer[x + y * width].z) + "m  ";
                } else if (x > 0 && z_buffer[x + y * width - 1] != 0) {
                    output_buf += "\033[m  ";
                } else {
                    output_buf += "  ";
                }
            }
            x = 0;
            if (bias)
                output_buf += "\033[m ";
        }
    }
}

void Screen::calculate_fps() {
    ++counter_t;
    if (clock() - start_t == 0) return;
    if (counter_t >= 100) {
        end_t = clock();
        fps = CLOCKS_PER_SEC * 100 / (end_t - start_t);
        start_t = clock();
        counter_t = 0;
    }
}

void Screen::show() const {
    std::cout << output_buf;
}