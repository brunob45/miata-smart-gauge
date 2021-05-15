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

    void put(float value)
    {
        _history += (value - _history) * _strength;
    }

    float get()
    {
        return _history;
    }
};

#endif // DASH_FILTER_H