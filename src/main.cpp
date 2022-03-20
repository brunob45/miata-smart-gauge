
#include <Arduino.h>

#include "accel.h"
#include "canbus.h"
#include "display.h"
#include "global.h"

static GlobalVars GV;

void chSetup()
{
    Accel::initThreads(NORMALPRIO + 3, (void*)&GV);
    CanBus::initThreads(NORMALPRIO + 2, (void*)&GV);
    Display::initThreads(NORMALPRIO + 1, (void*)&GV);
}

void setup()
{
    chBegin(chSetup);
}
void loop() {}