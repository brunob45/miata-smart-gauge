
#include "main.h"

GlobalVars GV;

IntervalTimer commTimer;

void setup()
{

    Serial.begin(115200);
    Serial1.begin(115200);

    Accel::init();
    Display::init();

    commTimer.begin(update_comms, 1000); // Do not increase above 5000us
}

void loop(void)
{
    Accel::update();
    update_values();
    Display::update();
}

void update_comms(void)
{
    for (int xfer_size = Serial.available(); xfer_size > 0; xfer_size--)
    {
        Serial1.write(Serial.read());
    }
    for (int xfer_size = Serial1.available(); xfer_size > 0; xfer_size--)
    {
        Serial.write(Serial1.read());
    }
}

void update_values()
{
    static uint32_t last_update = 0;
    static int16_t rpm_inc = 100;

    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    GV.rpm += rpm_inc;
    if (GV.rpm >= 8000 && rpm_inc > 0)
    {
        rpm_inc = -200;
    }
    else if (GV.rpm <= 200 && rpm_inc < 0)
    {
        rpm_inc = 100;
    }

    GV.gear = GV.rpm / 1335;

    GV.alert = GV.rpm > 7200;

    GV.accel = Accel::get().norm();

    last_update = now;
}
