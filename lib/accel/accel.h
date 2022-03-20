#if !defined(ACCEL_H)
#define ACCEL_H

#include <Arduino.h>
#include <ChRt.h>

namespace Accel
{
void initThreads(tprio_t prio = NORMALPRIO, void* arg = NULL);
size_t getUnusedStack();
} // namespace Accel

#endif // ACCEL_H