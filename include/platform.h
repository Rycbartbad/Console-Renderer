
#ifndef PROJECT_PLATFORM_H
#define PROJECT_PLATFORM_H

#include <chrono>
#include <thread>

// ── High-resolution stopwatch (cross-platform, replaces QPC) ─────────────
struct Stopwatch {
    std::chrono::high_resolution_clock::time_point start;

    Stopwatch() : start(std::chrono::high_resolution_clock::now()) {}

    [[nodiscard]] double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    void reset() { start = std::chrono::high_resolution_clock::now(); }
};

// ── Platform-specific console / input ────────────────────────────────────
#ifdef _WIN32

#include <windows.h>
#include <conio.h>

namespace platform {

inline void console_init() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hOut, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
    // Hide cursor
    printf("\033[?25l");
}

inline void console_get_size(int& width, int& height, bool& bias) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    csbi.dwSize.X = csbi.dwMaximumWindowSize.X;
    csbi.dwSize.Y = csbi.dwMaximumWindowSize.Y;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);
    bias = csbi.dwSize.X % 2 != 0;
    width = csbi.dwSize.X / 2;
    height = csbi.dwSize.Y;
#else
    winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    bias = false;
    width = ws.ws_col / 2;
    height = ws.ws_row;
#endif
}

inline void console_write(const char* data, size_t len) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    WriteConsoleA(hOut, data, static_cast<DWORD>(len), &written, nullptr);
}

inline bool kbhit() {
    return _kbhit() != 0;
}

inline int getch() {
    return _getch();
}

inline void get_cursor_pos(int& x, int& y) {
    POINT p;
    GetCursorPos(&p);
    x = p.x;
    y = p.y;
}

inline bool key_down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

inline void sleep_ms(int ms) {
    Sleep(static_cast<DWORD>(ms));
}

} // namespace platform

#else // Linux / macOS

#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>

// Virtual-key codes (cross-platform defines for the few used in the codebase)
#ifndef VK_RBUTTON
#define VK_RBUTTON 0x02
#endif

namespace platform {

inline void console_init() {
    // Hide cursor via ANSI
    printf("\033[?25l");
    fflush(stdout);
    // Switch stdin to raw mode for kbhit/getch
    static bool raw = false;
    if (!raw) {
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        raw = true;
    }
}

inline void console_get_size(int& width, int& height) {
    winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    width = ws.ws_col / 2;
    height = ws.ws_row;
}

inline bool console_has_bias() { return false; }

inline void console_write(const char* data, size_t len) {
    write(STDOUT_FILENO, data, len);
}

inline bool kbhit() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    char c;
    bool hit = read(STDIN_FILENO, &c, 1) > 0;
    if (hit) ungetc(c, stdin);  // push back for getch
    fcntl(STDIN_FILENO, F_SETFL, flags);
    return hit;
}

inline int getch() {
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

inline void get_cursor_pos(int& x, int& y) {
    x = y = 0;  // Not easily available on POSIX without termcap
}

inline bool key_down(int vk) {
    (void)vk; return false;  // No async key state on POSIX
}

inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace platform

#endif // _WIN32

#endif // PROJECT_PLATFORM_H
