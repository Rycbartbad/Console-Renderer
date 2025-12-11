
#ifndef PROJECT_GRAPHICS_H
#define PROJECT_GRAPHICS_H
#include "light.h"
#include "material.h"
#include "math_utils.h"
#include "screen.h"

class Line {
public:
    static void draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec3& color);

protected:
    static void fast_draw(Screen &screen, const Vec2 &a, const Vec2 &b, const Vec3 &color,
                        const Material& material, const std::vector<Light>& lights, const Vec2 *vertices, const Vec4* v, const Vec4& dir, const Vec4& normal);
};

class Triangle : public Line {
public:
    static void scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c,
                         const Vec3& color, const Material& material, const std::vector<Light>& lights, const Vec4* v, const Vec4& dir);

    static void scan_draw(Screen& screen, const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& color);

};
#endif //PROJECT_GRAPHICS_H