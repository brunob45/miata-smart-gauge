#ifndef DASH_SPEEDO_H
#define DASH_SPEEDO_H

#include <Arduino.h>
#include <ChRt.h>

namespace Speedo
{
void initThreads(tprio_t prio, void* arg);
size_t getUnusedStack();
} // namespace Speedo

#endif // DASH_SPEEDO_H