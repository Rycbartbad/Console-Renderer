cmake_minimum_required(VERSION 4.0)
project(Project)

set(CMAKE_CXX_STANDARD 20)

# Default to Release build for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

# Windows specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    set(WIN32_LIBS kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32)
    if(MSVC)
        add_compile_options(/utf-8)
    endif()
endif()

# ── Single-header render engine ───────────────────────────────────
# Include this header in your project. Define RENDER_ENGINE_IMPLEMENTATION
# in exactly ONE translation unit before including it.

# Main executable (stress test scene)
add_executable(Project src/main.cpp)

# Examples (build separately)
# add_executable(example_2048 example/2048.cpp)
# add_executable(example_galaxy example/galaxy.cpp)
# add_executable(example_stress example/stress_test.cpp)

# Headers
target_include_directories(Project PRIVATE include)
# target_include_directories(example_2048 PRIVATE include)
# target_include_directories(example_galaxy PRIVATE include)
# target_include_directories(example_stress PRIVATE include)

target_link_libraries(Project PRIVATE Threads::Threads ${WIN32_LIBS})
