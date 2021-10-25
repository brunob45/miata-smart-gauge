#ifndef DASH_FILTER_H
#define DASH_FILTER_H

#include <stdint.h>

class FilterClass
{
    const float _a;
    float _y0;
    float _e1;

public:
    constexpr FilterClass(float a)
        : _a(a), _y0(0.0f), _e1(0.0f)
    {
    }

    float reset(float x0 = 0.0f)
    {
        _y0 = x0;
        _e1 = 0.0f;
        return _y0;
    }

    float put(float x0)
    {
        float e0 = x0 - _y0;

        // as seen in a video from Phil’s Lab: https://youtu.be/zOByx3Izf5U?t=855
        _y0 += _a * (e0 + _e1) / 2.0f;
        // was _y0 += _a * e0;

        _e1 = e0;
        return _y0;
    }

    float get()
    {
        return _y0;
    }
};

#endif // DASH_FILTER_H