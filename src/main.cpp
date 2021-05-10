
#include <Arduino.h>

#include "accel.h"
#include "canbus.h"
#include "display.h"
#include "global.h"
#include "speedo.h"

GlobalVars GV;

// IntervalTimer taskTimer;

constexpr uint8_t lumi(uint8_t percent)
{
    unsigned int l = (percent * 255u) / 100u;
    return (l * l) / 256u;
}

elapsedMillis em;

void setup()
{
    em = 0;

    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    pinMode(A6, INPUT);

    Accel::init();
    // Speedo::init();
    CanBus::init();
    Display::init();

    // taskTimer.begin([] {}, 100); // 64 bytes @ 115200 baud is 4.44ms, so a check every 1ms is sufficient
}

void loop(void)
{
    Accel::update();
    // Speedo::update();
    CanBus::update();

    // GV.ms.rpm = Speedo::get_value();
    GV.alert = GV.ms.rpm > 7200;

    GV.lumi = analogRead(A6);
    analogWrite(6, (GV.lumi > 512) ? 30 : 255);

    Display::update();
}
