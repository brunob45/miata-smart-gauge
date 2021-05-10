#include "canbus.h"

#include <FlexCAN.h>

#include "global.h"

namespace CanBus
{
const uint32_t CANBUS_TIMEOUT = 2000;
const uint32_t CANBUS_SPEED = 500000;

FlexCAN CANbus(500000);
elapsedMillis em;

void init()
{
    // Enable CAN transceiver
    pinMode(23, OUTPUT);
    digitalWrite(23, LOW);

    // Start CAN driver
    CANbus.begin();

    Serial.begin(115200);
}

void update()
{
    for (int i = CANbus.available(); i > 0; i--)
    {
        CAN_message_t msg;
        CANbus.read(msg);

        // Convert each field from big endian to local format
        switch (msg.id)
        {
        case 1512:
            GV.ms.map = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.rpm = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.clt = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.tps = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            em = 0;
            break;
        case 1513:
            GV.ms.pw1 = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.pw2 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.mat = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.adv = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            em = 0;
            break;
        case 1514:
            GV.ms.afrtgt = msg.buf[0];
            GV.ms.afr = msg.buf[1];
            GV.ms.egocor = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.egt = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.pwseq = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            em = 0;
            break;
        case 1515:
            GV.ms.batt = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.sensors1 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.sensors2 = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.knk_rtd = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            em = 0;
            break;
        default:
            // unknown id
            break;
        }
    }
    GV.connected = (em < CANBUS_TIMEOUT); // Timeout after 5s
}
} // namespace CanBus