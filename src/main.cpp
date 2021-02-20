
#include "main.h"
#include "current_status.h"

#define COMM_SRC_NONE 0
#define COMM_SRC_LOCAL 1
#define COMM_SRC_USB 2

GlobalVars GV;

IntervalTimer commTimer;
int emSinceUsbData;
CurrentStatus cs;
uint8_t comm_source = COMM_SRC_NONE;
uint16_t comm_index = 0;

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);

    Accel::init();
    Display::init();

    commTimer.begin(update_comms, 1000); // 64 bytes @ 115200 baud is 4.44ms, so a check every 1ms is sufficient
}

void loop(void)
{
    static elapsedMillis em = 0;
    static uint16_t last_index = 0;

    Accel::update();
    update_values();
    Display::update();

    if (em > 50)
    {
        em = 0;
        if (comm_source == COMM_SRC_NONE)
        {
            comm_index = 0;
            last_index = 0;
            comm_source = COMM_SRC_LOCAL;
            Serial1.write('r');
        }
        else if (comm_source == COMM_SRC_LOCAL && comm_index == last_index)
        {
            // timeout
            comm_source = COMM_SRC_NONE;
        }
    }
}

void update_comms(void)
{
    if (emSinceUsbData >= 500)
    {
        comm_source = COMM_SRC_NONE;
    }
    else
    {
        ++emSinceUsbData;
    }
    for (int xfer_size = Serial.available(); xfer_size > 0; xfer_size--)
    {
        emSinceUsbData = 0;
        comm_source = COMM_SRC_USB;
        Serial1.write(Serial.read());
    }
    for (int xfer_size = Serial1.available(); xfer_size > 0; xfer_size--)
    {
        if (comm_source == COMM_SRC_USB)
        {
            Serial.write(Serial1.read());
        }
        else if (comm_source == COMM_SRC_LOCAL)
        {
            ((uint8_t*)&cs)[comm_index++] = Serial1.read();
            if (comm_index >= sizeof(CurrentStatus)) { comm_source = COMM_SRC_NONE; }
        }
        else // comm_source == COMM_SRC_NONE
        {
            Serial1.read(); // flush buffer
        }
    }
}

void update_values()
{
    static uint32_t last_update = 0;
    static int16_t rpm_inc = 100;

    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    // GV.rpm += rpm_inc;
    // if (GV.rpm >= 8000 && rpm_inc > 0)
    // {
    //     rpm_inc = -200;
    // }
    // else if (GV.rpm <= 200 && rpm_inc < 0)
    // {
    //     rpm_inc = 100;
    // }
    GV.rpm = cs.rpm;
    GV.map = cs.map;
    GV.pw = cs.pw;

    GV.gear = GV.rpm / 1335;

    GV.alert = GV.rpm > 7200;

    GV.accel = Accel::get().norm();

    last_update = now;
}
