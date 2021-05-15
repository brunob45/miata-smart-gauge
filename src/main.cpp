
#include <Arduino.h>

#define COMM_SRC_NONE 0
#define COMM_SRC_LOCAL 1
#define COMM_SRC_USB 2

#include "accel.h"
#include "canbus.h"
#include "current_status.h"
#include "display.h"
#include "global.h"
#include "speedo.h"

GlobalVars GV;

IntervalTimer commTimer;
volatile int last_index;
CurrentStatus cs;
uint8_t comm_source = COMM_SRC_NONE;
uint16_t comm_index = 0;

void setup()
{
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    pinMode(A6, INPUT);

    Serial.begin(115200);
    Serial1.begin(115200);

    Accel::init();
    // Speedo::init();
    CanBus::init();
    Display::init();

    // commTimer.begin(update_comms, 1000); // 64 bytes @ 115200 baud is 4.44ms, so a check every 1ms is sufficient
    // last_index = 0;
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

void update_comms(void)
{
    static bool usb_ongoing = false, local_ongoing = false;
    static int update_counter = 0, last_usb_receive = 0;
    ++update_counter;
    ++last_usb_receive;

    if (usb_ongoing)
    {
        for (int xfer_size = Serial.available(); xfer_size > 0; xfer_size--)
        {
            last_usb_receive = 0;
            Serial1.write(Serial.read());
        }
        for (int xfer_size = Serial1.available(); xfer_size > 0; xfer_size--)
        {
            last_usb_receive = 0;
            Serial.write(Serial1.read());
        }
        if (last_usb_receive > 500)
        {
            usb_ongoing = false;
        }
    }
    else if (local_ongoing)
    {
        for (int xfer_size = Serial1.available(); xfer_size > 0; xfer_size--)
        {
            ((uint8_t*)&cs)[comm_index++] = Serial1.read();
            last_index = comm_index;
            local_ongoing = comm_index < sizeof(CurrentStatus);
        }
        if (update_counter > 100)
        {
            // timeout
            cs.rpm = 0;
            local_ongoing = false;
        }
    }
    else
    {
        if (Serial.available())
        {
            usb_ongoing = true;
        }
        else if (update_counter > 50)
        {
            local_ongoing = true;
            update_counter = 0;
            comm_index = 0;
            last_index = 0;
            Serial1.write('r');
        }
    }
}
