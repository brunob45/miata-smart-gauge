#if !defined(DASH_ACCEL_H)
#define DASH_ACCEL_H

#include <Arduino.h>

namespace Accel
{
struct AccelValue
{
    float x;
    float y;
    float z;

    float norm();
};

void init(void);
void update(void);
void print_debug(Print& p);
AccelValue get();
} // namespace Accel

#endif // DASH_ACCEL_H