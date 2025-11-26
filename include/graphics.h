
#ifndef PROJECT_GRAPHICS_H
#define PROJECT_GRAPHICS_H
#include "math_utils.h"
#include "screen.h"

class Line {
public:
    static void draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color);

protected:
    static void fast_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color,
                         const Vec2* vertices, const float* v_z, float f, float n);
};

class Triangle : public Line {
public:
    static void scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c,
                         const Vec3& color, const float* v_z, float f, float n);
};
#endif //PROJECT_GRAPHICS_H