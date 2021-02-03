#if !defined(DASH_POINT_H)
#define DASH_POINT_H

#include <Arduino.h>

class Point
{
    int16_t _x, _y;

public:
    enum Coordinates
    {
        CARTESIAN,
        POLAR
    };

    Point() : _x(0), _y(0) {}
    Point(int16_t x, int16_t y, Coordinates type = CARTESIAN)
    {
        switch (type)
        {
        case CARTESIAN:
            _x = x;
            _y = y;
            break;
        case POLAR:
            _x = sin(radians(x)) * y;
            _y = cos(radians(x)) * y;
            break;
        }
    }

    int16_t x() { return _x; }
    int16_t y() { return _y; }

    Point operator+(const Point& rhs)
    {
        return Point(_x + rhs._x, _y + rhs._y);
    }
    Point operator-(const Point& rhs)
    {
        return Point(_x - rhs._x, _y - rhs._y);
    }
};

#endif // DASH_POINT_H