
#include <Arduino.h>

#include "accel.h"
#include "display.h"

void chSetup()
{
    Accel::initThreads(NORMALPRIO + 2);
    Display::initThreads(NORMALPRIO + 1);
}

void setup()
{
    chBegin(chSetup);
}
void loop() {}