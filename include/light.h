
#ifndef PROJECT_LIGHT_H
#define PROJECT_LIGHT_H
#include "math_utils.h"

struct Light {
    Vec4 pos;
    Vec3 color;
    float intensity{};
};
#endif //PROJECT_LIGHT_H