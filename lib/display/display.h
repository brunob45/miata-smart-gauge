#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <ChRt.h>

namespace Display
{
void initThreads(tprio_t prio = NORMALPRIO);
}

#endif // DISPLAY_H