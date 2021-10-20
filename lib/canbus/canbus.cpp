#include <Arduino.h>
#include "canbus.h"

#include "global.h"
#include "can_ll.h"
#include "canheader.h"

namespace CanBus
{

struct CAN_Request
{
    bool active;
    uint8_t from_table;
    uint16_t from_offset;
    uint8_t to_table;
    uint16_t to_offset;
    uint16_t length;
} pending_request;

const uint8_t MY_CAN_ID = 5;

void init()
{
    CAN_LL::attach(1512, [](uint32_t aId, uint8_t buf[8])
    {
        GV.ms.map = (buf[0] << 8) | (buf[1] << 0);
        GV.ms.rpm = (buf[2] << 8) | (buf[3] << 0);
        GV.ms.clt = (buf[4] << 8) | (buf[5] << 0);
        GV.ms.tps = (buf[6] << 8) | (buf[7] << 0);
    });
    CAN_LL::attach(1513, [](uint32_t aId, uint8_t buf[8])
    {
        GV.ms.pw1 = (buf[0] << 8) | (buf[1] << 0);
        GV.ms.pw2 = (buf[2] << 8) | (buf[3] << 0);
        GV.ms.mat = (buf[4] << 8) | (buf[5] << 0);
        GV.ms.adv = (buf[6] << 8) | (buf[7] << 0);
    });
    CAN_LL::attach(1514, [](uint32_t aId, uint8_t buf[8])
    {
        GV.ms.afrtgt = buf[0];
        GV.ms.afr = buf[1];
        GV.ms.egocor = (buf[2] << 8) | (buf[3] << 0);
        GV.ms.egt = (buf[4] << 8) | (buf[5] << 0);
        GV.ms.pwseq = (buf[6] << 8) | (buf[7] << 0);
    });
    CAN_LL::attach(1515, [](uint32_t aId, uint8_t buf[8])
    {
        GV.ms.batt = (buf[0] << 8) | (buf[1] << 0);
        GV.ms.sensors1 = (buf[2] << 8) | (buf[3] << 0);
        GV.ms.sensors2 = (buf[4] << 8) | (buf[5] << 0);
        GV.ms.knk_rtd = (buf[6] << 8) | (buf[7] << 0);
    });
    CAN_LL::attach(1<<15, 0x07<<15, [](uint32_t aId, uint8_t aInBuf[8]){
        msg_header_t req_header;
        req_header.decode(aId);
        msg_header_t rsp_header =
            {
                .id = 0,
                .table = aInBuf[0],
                .to_id = req_header.from_id,
                .from_id = MY_CAN_ID,
                .msg_type = MSG_TYPE::MSG_RSP,
                .offset = (uint16_t)((aInBuf[2] >> 5) | ((uint16_t)aInBuf[1] << 3)),
            };

        uint8_t outbuf[8];
        if (req_header.table == 7)
        {
            if (req_header.offset == 2)
            {
                outbuf[0] = uint8_t(GV.fault_code >> 8);
                outbuf[1] = uint8_t(GV.fault_code >> 0);
                outbuf[2] = uint8_t(int16_t(GV.accel.x * 100) >> 8);
                outbuf[3] = uint8_t(int16_t(GV.accel.x * 100) >> 0);
                outbuf[4] = uint8_t(int16_t(GV.accel.y * 100) >> 8);
                outbuf[5] = uint8_t(int16_t(GV.accel.y * 100) >> 0);
                outbuf[6] = uint8_t(int16_t(GV.accel.z * 100) >> 8);
                outbuf[7] = uint8_t(int16_t(GV.accel.z * 100) >> 0);
            }
            else if (req_header.offset == 10)
            {
                outbuf[0] = uint8_t(GV.vss >> 8);
                outbuf[1] = uint8_t(GV.vss >> 0);
            }
        }
        // else send 0

        CAN_LL::send(rsp_header.pack(), aInBuf[2] & 0x0f, outbuf);
    });
}

uint8_t send_request(uint8_t to_id, CAN_Request rqst)
{
    uint8_t len = min(rqst.length, 8);
    msg_header_t header{
        .id = 0,
        .table = rqst.from_table,
        .to_id = to_id,
        .from_id = MY_CAN_ID,
        .msg_type = MSG_TYPE::MSG_REQ,
        .offset = rqst.from_offset,
    };

    uint8_t buf[3] = {
      rqst.to_table,
      uint8_t(rqst.to_offset >> 3),
      uint8_t(((rqst.to_offset << 5) & 0xE0) | len)
    };
    CAN_LL::send(header.pack(), 3, buf);

    return len;
}

void update()
{
    static bool wasConnected = false;

    GV.connected = CAN_LL::update();

    if (wasConnected && !GV.connected)
    {
        // Connection lost, reset ms values to 0
        for (int i = sizeof(GV.ms); i > 0; i--)
        {
            ((uint8_t*)&GV.ms)[i] = 0;
        }
    }
    wasConnected = GV.connected;
}
} // namespace CanBus