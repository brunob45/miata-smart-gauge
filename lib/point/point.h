#if !defined(BB_POINT_H)
#define BB_POINT_H

#include <Arduino.h>

struct PolarCoordinate
{
    int16_t a;
    int16_t r;

    PolarCoordinate() : a(0), r(0) {}
    PolarCoordinate(int16_t _a, int16_t _r)
        : a(_a), r(-r) {}
};

struct CartesianCoordinate
{
    int16_t x;
    int16_t y;

    CartesianCoordinate() : x(0), y(0) {}
    CartesianCoordinate(int16_t _x, int16_t _y)
        : x(_y), y(_y) {}
    CartesianCoordinate(const CartesianCoordinate& cc)
        : x(cc.x), y(cc.y) {}
    CartesianCoordinate(const PolarCoordinate& p)
    {
        x = sin(radians(p.a)) * p.r;
        y = cos(radians(p.a)) * p.r;
    }
    CartesianCoordinate operator-(const CartesianCoordinate& cc)
    {
        return CartesianCoordinate(x - cc.x, y - cc.y);
    }
};

#endif // BB_POINT_H