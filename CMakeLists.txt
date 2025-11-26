cmake_minimum_required(VERSION 4.0)
project(Project)

set(CMAKE_CXX_STANDARD 20)

include_directories(${INC_DIR})
link_directories(${LINK_DIR})

find_package(Threads REQUIRED)

# Windows特定设置
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    set(WIN32_LIBS kernel32 user32 gdi32 winspool shell32 ole32 oleaut32 uuid comdlg32 advapi32)
endif()

# 源文件列表
set(SOURCES
        src/main.cpp
        src/transform.cpp
        src/mesh.cpp
        src/screen.cpp
        src/camera.cpp
        src/graphics.cpp
        src/render.cpp
        src/rubik_cube.cpp
)

# 头文件目录
include_directories(include)

# 创建可执行文件
add_executable(Project ${SOURCES})

