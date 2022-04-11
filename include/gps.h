#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <ChRt.h>

namespace GPS
{
void initThreads(tprio_t prio = NORMALPRIO, void* arg = NULL);
}

#endif // GPS_H