
#ifndef PROJECT_MATH_UTILS_H
#define PROJECT_MATH_UTILS_H

#include <cmath>

constexpr float pi = 3.14159f;

class Vec2 {
public:
    Vec2() : x(0), y(0) {}
    Vec2(const int x, const int y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& b) const { return { x + b.x, y + b.y }; }
    Vec2 operator-(const Vec2& b) const { return { x - b.x, y - b.y }; }
    Vec2 operator*(int b) const { return { x * b, y * b }; }

    int cross(const Vec2& b) const { return x * b.y - y * b.x; }

    int x, y;
};

class Vec3 {
public:
    Vec3() : x(0), y(0), z(0) {}
    Vec3(const int x, const int y, const int z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& b) const { return { x + b.x, y + b.y, z + b.z }; }
    Vec3 operator-(const Vec3& b) const { return { x - b.x, y - b.y, z - b.z }; }
    Vec3 operator*(const int b) const { return { x * b, y * b, z * b }; }
    Vec3 operator*(const float b) const { return { static_cast<int>(x * b), static_cast<int>(y * b), static_cast<int>(z * b) }; }
    Vec3 operator/(const int b) const { return { x / b, y / b, z / b }; }
    [[nodiscard]] Vec3 hadamard(const Vec3& b) const { return {x * b.x, y * b.y, z * b.z}; }
    int operator*(const Vec3& b) const { return x * b.x + y * b.y + z * b.z; }

    bool operator==(const Vec3& b) const { return x == b.x && y == b.y && z == b.z; }

    int x, y, z;
};

class Vec4 {
public:
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(const float x, const float y, const float z, const float w) : x(x), y(y), z(z), w(w) {}

    Vec4 operator+(const Vec4& b) const { return { x + b.x, y + b.y, z + b.z, 1 }; }
    Vec4 operator-(const Vec4& b) const { return { x - b.x, y - b.y, z - b.z, 1 }; }
    Vec4 operator*(float b) const { return { x * b, y * b, z * b, w * b }; }
    float operator*(const Vec4& b) const { return x * b.x + y * b.y + z * b.z + w * b.w; }

    Vec3 to_vec3() const { return { static_cast<short>(round(x)), static_cast<short>(round(y)), static_cast<short>(round(z)) }; }

    void normalize() {
        if (const float sum = sqrt(x * x + y * y + z * z); sum > 0.00001f) {
            x /= sum; y /= sum; z /= sum;
        }
        w = 0;
    }

    [[nodiscard]] Vec4 cross3(const Vec4& b) const {
        return { y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x, 1 };
    }

    Vec4 operator/(float b) const {
        return {x / b, y / b, z / b, w / b};
    };

    float x, y, z, w;
};

class Mat4 {
public:
    Mat4() : mat{Vec4(1,0,0,0), Vec4(0,1,0,0), Vec4(0,0,1,0), Vec4(0,0,0,1)} {}
    Mat4(const Vec4& x, const Vec4& y, const Vec4& z, const Vec4& w) : mat{x, y, z, w} {}

    Vec4 operator*(const Vec4& b) const {
        return {
            mat[0].x * b.x + mat[0].y * b.y + mat[0].z * b.z + mat[0].w * b.w,
            mat[1].x * b.x + mat[1].y * b.y + mat[1].z * b.z + mat[1].w * b.w,
            mat[2].x * b.x + mat[2].y * b.y + mat[2].z * b.z + mat[2].w * b.w,
            mat[3].x * b.x + mat[3].y * b.y + mat[3].z * b.z + mat[3].w * b.w
        };
    }

    Mat4 operator*(const Mat4& b) const {
        Mat4 temp;
        for (int i = 0; i < 4; i++) {
            temp.mat[i].x = mat[i].x * b.mat[0].x + mat[i].y * b.mat[1].x + mat[i].z * b.mat[2].x + mat[i].w * b.mat[3].x;
            temp.mat[i].y = mat[i].x * b.mat[0].y + mat[i].y * b.mat[1].y + mat[i].z * b.mat[2].y + mat[i].w * b.mat[3].y;
            temp.mat[i].z = mat[i].x * b.mat[0].z + mat[i].y * b.mat[1].z + mat[i].z * b.mat[2].z + mat[i].w * b.mat[3].z;
            temp.mat[i].w = mat[i].x * b.mat[0].w + mat[i].y * b.mat[1].w + mat[i].z * b.mat[2].w + mat[i].w * b.mat[3].w;
        }
        return temp;
    }

private:
    Vec4 mat[4];
};
#endif //PROJECT_MATH_UTILS_H