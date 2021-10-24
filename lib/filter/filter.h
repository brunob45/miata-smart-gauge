#ifndef DASH_FILTER_H
#define DASH_FILTER_H

#include <stdint.h>

class FilterClass
{
    const float _strength;
    float _history;

public:
    constexpr FilterClass(float strength)
        : _strength(strength), _history(0.0f)
    {
    }

    float reset(float value = 0.0f)
    {
        _history = value;
        return _history;
    }

    float put(float value)
    {
        _history += (value - _history) * _strength;
        return _history;
    }

    float get()
    {
        return _history;
    }
};

#endif // DASH_FILTER_H