
#include <Arduino.h>

#include "accel.h"
#include "canbus.h"
#include "display.h"
#include "global.h"
#include "gps.h"
#include "speedo.h"

static GlobalVars GV;

void chSetup()
{
    Speedo::initThreads(NORMALPRIO + 5, (void*)&GV); // 50ms
    Accel::initThreads(NORMALPRIO + 4, (void*)&GV);  // 20ms
    GPS::initThreads(NORMALPRIO + 3, (void*)&GV);    // 20ms

    CanBus::initThreads(NORMALPRIO + 2, (void*)&GV); // 1ms

    Display::initThreads(NORMALPRIO + 1, (void*)&GV); // 20ms
}

void setup()
{
    chBegin(chSetup);
}
void loop() {}