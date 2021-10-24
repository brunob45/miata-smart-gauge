#ifndef DASH_FILTER_H
#define DASH_FILTER_H

#include <stdint.h>

class FilterClass
{
    const float _a;
    float _y0;
    float _x1;

public:
    constexpr FilterClass(float a)
        : _a(a), _y0(0.0f), _x1(0.0f)
    {
    }

    float reset(float x0 = 0.0f)
    {
        _y0 = _x1 = x0;
        return _y0;
    }

    float put(float x0)
    {
#if IMPROVED_FILTER
        // as seen at https://dsp.stackexchange.com/a/60286
        float avg = (x0 + _x1) / 2;
        _y0 += (avg - _y0) * _a;
#else
        _y0 += (x0 - _y0) * _a;
#endif
        _x1 = x0;
        return _y0;
    }

    float get()
    {
        return _y0;
    }
};

#endif // DASH_FILTER_H