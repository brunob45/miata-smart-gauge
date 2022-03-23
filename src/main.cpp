
#include <Arduino.h>

#include "accel.h"
#include "canbus.h"
#include "display.h"
#include "global.h"
#include "speedo.h"

static GlobalVars GV;

void chSetup()
{
    Accel::initThreads(NORMALPRIO + 4, (void*)&GV);   // 20ms
    Speedo::initThreads(NORMALPRIO + 3, (void*)&GV);  // 50ms
    CanBus::initThreads(NORMALPRIO + 2, (void*)&GV);  // 1ms
    Display::initThreads(NORMALPRIO + 1, (void*)&GV); // 20ms
}

void setup()
{
    chBegin(chSetup);
}
void loop() {}