
#include "main.h"

DisplayClass Display(6);
AccelClass Accel;
GVStruct GV;

int16_t rpm_inc = 100;

void setup()
{
    Accel.init();
    Display.init();
}

void loop(void)
{
    Accel.update();
    update_values();
    Display.update();
}

void update_values()
{
    static uint32_t last_update = 0;
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

    GV.current_gear = GV.rpm / 1335;

    GV.alert = GV.rpm >= 7200;

    last_update = now;
}
