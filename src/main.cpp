
#include <Arduino.h>

#include "display.h"

void chSetup()
{
    Display::initThreads(NORMALPRIO);
}

void setup()
{
    chBegin(chSetup);
}
void loop() {}