#ifndef BB_FILTER_H
#define BB_FILTER_H

#include <stdint.h>

template <typename T, int FILTER_SIZE>
class FilterClass
{
    T history[FILTER_SIZE];
    int last_index;

public:
    FilterClass()
        : last_index(0)
    {
        for (int i = 0; i < FILTER_SIZE; i++)
        {
            history[i] = 0;
        }
    }

    void put(T value)
    {
        history[last_index++] = value;
        if (last_index == FILTER_SIZE)
        {
            last_index = 0;
        }
    }

    T get()
    {
        T acc = 0;
        for (int i = 0; i < FILTER_SIZE; i++)
        {
            acc += history[i];
        }
        return acc / FILTER_SIZE;
    }
};

#endif // BB_FILTER_H