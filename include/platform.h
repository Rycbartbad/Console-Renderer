
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
#include <memory>

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

// Cooperative yield — yields the remainder of the current timeslice (~0μs delay).
// On Windows, Sleep(0) is more efficient than std::this_thread::yield() because
// it allows the scheduler to run ANY waiting thread, even lower-priority ones.
inline void yield_thread() {
    Sleep(0);
}

// Returns the number of physical cores (excludes hyperthreads, E-cores).
// Worker threads = physical cores avoids contention from SMT / hybrid CPU architectures.
inline int ideal_thread_count() {
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || len == 0)
        return std::thread::hardware_concurrency() / 2;
    auto buf = std::make_unique<SYSTEM_LOGICAL_PROCESSOR_INFORMATION[]>(len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(buf.get(), &len))
        return std::thread::hardware_concurrency() / 2;
    int cores = 0;
    for (DWORD i = 0; i < len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++) {
        if (buf[i].Relationship == RelationProcessorCore) cores++;
    }
    return std::max(1, cores);
}

// Pin calling thread to a specific CPU core (0-based).  Helps prevent slow
// E-cores from becoming the bottleneck in hybrid architectures.
inline void set_thread_affinity(int core_index) {
    HANDLE hThread = GetCurrentThread();
    SetThreadAffinityMask(hThread, static_cast<DWORD_PTR>(1) << core_index);
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
    // Query cursor position via ANSI DSR \x1b[6n — terminal responds \x1b[row;colR
    // This gives the text cursor position as an approximation of mouse focus.
    fflush(stdout);
    write(STDOUT_FILENO, "\x1b[6n", 4);
    
    // Temporarily set stdin to blocked-but-timed mode to read the response
    struct termios old, tmp;
    tcgetattr(STDIN_FILENO, &old);
    tmp = old;
    tmp.c_lflag &= ~(ICANON | ECHO);
    tmp.c_cc[VMIN] = 0;    // non-blocking minimum
    tmp.c_cc[VTIME] = 1;   // 100 ms timeout
    tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
    
    char buf[32];
    int pos = 0;
    while (pos < 31) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;
        buf[pos++] = c;
        if (c == 'R') break;  // end of DSR response
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    buf[pos] = '\0';
    
    int row = 0, col = 0;
    if (sscanf(buf, "\x1b[%d;%d", &row, &col) >= 2) {
        x = col / 2;   // half-width character cells
        y = row - 1;   // 0-based row
    } else {
        x = y = 0;
    }
}

inline bool key_down(int vk) {
    // Track pressed keys by consuming pending stdin in non-blocking mode.
    // Since terminals don't send key-release events, each press is tracked
    // for ~150 ms then auto-cleared — good enough for WASD movement.
    static bool state[256] = {false};
    static std::chrono::steady_clock::time_point press_time[256];
    static constexpr auto HOLD_MS = std::chrono::milliseconds(150);

    auto now = std::chrono::steady_clock::now();

    // Flush pending stdin into key state
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    char c;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        unsigned char uc = static_cast<unsigned char>(c);
        // Arrow/function keys send \x1b[ ... sequences — ignore non-printable
        if (uc >= 32 && uc <= 126) {
            state[uc] = true;
            press_time[uc] = now;
        }
    }
    fcntl(STDIN_FILENO, F_SETFL, flags);

    // Auto-clear expired holds
    unsigned char uc = static_cast<unsigned char>(vk);
    if (state[uc] && now - press_time[uc] > HOLD_MS)
        state[uc] = false;

    return state[uc];
}

inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Physical-core heuristic: assume SMT → divide by 2.
// On Linux /proc/cpuinfo could give exact count, but this is good enough.
inline int ideal_thread_count() {
    int logical = static_cast<int>(std::thread::hardware_concurrency());
    return std::max(1, logical / 2);
}

// No-op on POSIX — thread affinity requires pthread_setaffinity_np (Linux) or
// thread_policy_set (macOS).  The core-count heuristic already avoids E-core
// bottlenecks on hybrid CPUs because ideal_thread_count returns physical cores.
inline void set_thread_affinity(int) {}

inline void yield_thread() {
    std::this_thread::yield();
}

} // namespace platform

#endif // _WIN32

#endif // PROJECT_PLATFORM_H
