#include "rubik_cube.h"
#include "transform.h"
#include "mesh.h"
#include <conio.h>
#include <windows.h>

RubikCube::RubikCube(float length) {
    const float len = length / 3;
    for (int i = 0; i < 27; ++i) {
        meshes.emplace_back(gen_block(Vec4(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1, 1), len));
        block_indices[i] = Vec3(i % 3 - 1, i / 3 % 3 - 1, i / 9 - 1);
    }
}

Mesh RubikCube::gen_block(const Vec4& pos, float len) {
    const Vec3 color[6] = {
        Vec3(255,0,0),    // 红
        Vec3(0,255,0),    // 绿
        Vec3(255,88,0),   // 橙
        Vec3(0,0,255),    // 蓝
        Vec3(255,255,0),  // 黄
        Vec3(255,255,255),// 白
    };
    
    std::vector<Vec3> colors = {6, Vec3(200,200,200)}; // 默认灰色
    
    if (pos.x == -1)
        colors[3] = color[3]; // 左面蓝色
    else if (pos.x == 1)
        colors[1] = color[1]; // 右面绿色
        
    if (pos.y == -1)
        colors[5] = color[5]; // 下面白色
    else if (pos.y == 1)
        colors[4] = color[4]; // 上面黄色
        
    if (pos.z == -1)
        colors[0] = color[0]; // 后面红色
    else if (pos.z == 1)
        colors[2] = color[2]; // 前面橙色
        
    return Mesh::Cube(pos * (len + 0.1f), len / 2, colors);
}

void RubikCube::control(Vec3* blocks, const std::vector<Mesh*> &meshes) {
    static Vec4 axis_base[3] = {
        Vec4(1, 0, 0, 0),
        Vec4(0, 1, 0, 0),
        Vec4(0, 0, 1, 0)
    };
    
    if (_kbhit()) {
        bool rotate = false;
        Vec4 axis;
        std::vector<int> blk;
        
        switch (_getch()) {
            case 'f': // 前面
                axis = axis_base[2] * (-1);
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].z == -1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(0, 0, -1, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            case 'b': // 后面
                axis = axis_base[2];
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].z == 1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(0, 0, 1, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            case 'l': // 左面
                axis = axis_base[0] * (-1);
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].x == -1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(-1, 0, 0, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            case 'r': // 右面
                axis = axis_base[0];
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].x == 1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(1, 0, 0, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            case 'd': // 下面
                axis = axis_base[1] * (-1);
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].y == -1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(0, -1, 0, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            case 'u': // 上面
                axis = axis_base[1];
                for (int i = 0; i < 27; ++i) {
                    if (blocks[i].y == 1) {
                        blk.emplace_back(i);
                        Transform::rotate(blocks[i], Vec4(0, 1, 0, 0), pi/2);
                    }
                }
                rotate = true;
                break;
            default:
                break;
        }
        
        if (rotate) {
            constexpr int times = 1;
            for (int n = 0; n < 9; ++n) {
                for (const auto& i : blk) {
                    Transform::rotate(*meshes[i], axis, pi/18);
                }
                Sleep(times);
            }
        }
    }
    
    static POINT p1, p2;
    GetCursorPos(&p1);
    const float angel_x = (p2.x - p1.x) * pi / 360.0f;
    const float angel_y = (p2.y - p1.y) * pi / 360.0f;
    p2 = p1;

    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        for (auto& axis : axis_base) {
            Transform::rotate(axis, Vec4(0, 1, 0, 0), angel_x);
            Transform::rotate(axis, Vec4(1, 0, 0, 0), angel_y);
        }
        for (auto& mesh : meshes) {
            Transform::rotate(*mesh, Vec4(0, 1, 0, 0), angel_x);
            Transform::rotate(*mesh, Vec4(1, 0, 0, 0), angel_y);
        }
    }
}