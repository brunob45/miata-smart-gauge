#ifndef DASH_CANBUS
#define DASH_CANBUS

#include <Arduino.h>
#include <ChRt.h>

namespace CanBus
{
void initThreads(tprio_t prio = NORMALPRIO, void* arg = NULL);
size_t getUnusedStack();
} // namespace CanBus

#endif // DASH_CANBUS