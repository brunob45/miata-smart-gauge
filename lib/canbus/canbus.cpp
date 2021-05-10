#include "canbus.h"

#include <FlexCAN.h>

#include "global.h"

namespace CanBus
{
FlexCAN can0;

void init()
{
    can0.begin(500e3);
}

void update()
{
    for (int i = can0.available(); i > 0; i--)
    {
        CAN_message_t msg;
        can0.read(msg);

        // Convert each field from big endian to local format
        switch (msg.id)
        {
        case 1512:
            GV.ms.map = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.rpm = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.clt = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.tps = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            break;
        case 1513:
            GV.ms.pw1 = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.pw2 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.mat = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.adv = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            break;
        case 1514:
            GV.ms.afrtgt = msg.buf[0];
            GV.ms.afr = msg.buf[1];
            GV.ms.egocor = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.egt = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.pwseq = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            break;
        case 1515:
            GV.ms.batt = (msg.buf[0] << 8) | (msg.buf[1] << 0);
            GV.ms.sensors1 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
            GV.ms.sensors2 = (msg.buf[4] << 8) | (msg.buf[5] << 0);
            GV.ms.knk_rtd = (msg.buf[6] << 8) | (msg.buf[7] << 0);
            break;
        default:
            // unknown id
            break;
        }
    }
}
} // namespace CanBus