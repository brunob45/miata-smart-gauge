#ifndef FAST_MICROS_H
#define FAST_MICROS_H

#include <Arduino.h>

inline uint32_t fast_micros()
{
    return ARM_DWT_CYCCNT / (F_CPU / 1000000UL);
}

#endif // FAST_MICROS_H