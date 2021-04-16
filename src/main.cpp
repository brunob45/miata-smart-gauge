
#include <Arduino.h>

#include "accel.h"
#include "display.h"
#include "global.h"
#include "rpm.h"

GlobalVars GV;

// IntervalTimer taskTimer;

constexpr uint8_t lumi(uint8_t percent)
{
    unsigned int l = (percent * 255u) / 100u;
    return (l * l) / 256u;
}

uint8_t lumi_low = lumi(50);
uint8_t lumi_high = lumi(100);

void setup()
{
    pinMode(6, OUTPUT);
    analogWrite(6, 0);
    pinMode(A6, INPUT);

    Accel::init();
    RPM::init();
    Display::init();

    analogWrite(6, lumi_low);

    // taskTimer.begin([] {}, 100); // 64 bytes @ 115200 baud is 4.44ms, so a check every 1ms is sufficient
}

void loop(void)
{
    Accel::update();
    RPM::update();

    GV.rpm = RPM::get_value();
    GV.alert = GV.rpm > 7200;

    Accel::AccelValue a = Accel::get();
    GV.accel[0] = a.x;
    GV.accel[1] = a.y;
    GV.accel[2] = a.z;

    GV.lumi = analogRead(A6);

    analogWrite(6, 255 - (GV.lumi / 4u));

    Display::update();
}
