#if !defined(ACCEL_H)
#define ACCEL_H

#include <Arduino.h>
#include <ChRt.h>

namespace Accel
{
struct AccelValue
{
    float x;
    float y;
    float z;

    float norm();
};
AccelValue get();

void initThreads(tprio_t prio = NORMALPRIO);
size_t getUnusedStack();
} // namespace Accel

#endif // ACCEL_H