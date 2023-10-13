#ifndef __FILTER_WINDOW_H__
#define __FILTER_WINDOW_H__

template <typename T, int N>
class FilterWindow
{
    T buffer[N];
    T sum;
    int index;

public:
    FilterWindow() : sum(0), index(0) {}

    T operator()(T input)
    {
        sum -= buffer[index];
        buffer[index] = input;
        sum += buffer[index];

        if (index == 0) { index = N; }
        index -= 1;

        return sum;
    }
};

#endif // __FILTER_WINDOW_H__