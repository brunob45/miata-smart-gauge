#include "canbus.h"

#include <FlexCAN.h>

#include "global.h"

namespace CanBus
{
enum class MSG_TYPE : uint8_t
{
    MSG_CMD = 0,    // A 'poke' message to deposit data into memory.
    MSG_REQ = 1,    // A request message for data
    MSG_RSP = 2,    // A reply to a request, effectively the same as MSG_CMD.
    MSG_XSUB = 3,   // Not implemented
    MSG_BURN = 4,   // A request to burn data to flash.
    OUTMSG_REQ = 5, // A request for an outmsg set of data.
    OUTMSG_RSP = 6, // A response of an outmsg set of data
    MSG_XTND = 7,   // An extended message, message number is in first data byte.

    // extended message type
    MSG_FWD = 8,      // Message to send data out of the serial port.
    MSG_CRC = 9,      // Request for the CRC of a data table.
    MSG_REQX = 12,    // A request message for data (tables 16-31)
    MSG_BURNACK = 14, // Response message for burn.
    MSG_PROT = 0x80,  // Command for getting the protocol version number
    MSG_WCR = 0x81,   // Not implemented
    MSG_SPND = 0x82   // Command for suspending and resuming CAN polling to a device.
};

struct msg_header_t
{
    uint8_t table;
    uint8_t to_id;
    uint8_t from_id;
    MSG_TYPE msg_type;
    uint16_t offset;

    void decode(uint32_t can_id)
    {
        table = (can_id >> 3) & 0x0f;
        table |= ((can_id << 2) & 0x10); // add last bit to table index

        to_id = (can_id >> 7) & 0x0f;
        from_id = (can_id >> 11) & 0x0f;
        msg_type = (MSG_TYPE)((can_id >> 15) & 0x07);
        offset = (can_id >> 18) & 0x3f;
    }

    uint32_t pack()
    {
        uint32_t can_id = 0;

        can_id |= ((uint32_t)table & 0x10) >> 2;
        can_id |= ((uint32_t)table & 0x0f) << 3;
        can_id |= ((uint32_t)to_id & 0x0f) << 7;
        can_id |= ((uint32_t)from_id & 0x0f) << 11;
        can_id |= ((uint32_t)msg_type & 0x07) << 15;
        can_id |= ((uint32_t)offset & 0x3f) << 18;

        return can_id;
    }
};

const uint32_t CANBUS_TIMEOUT = 2000;
const uint32_t CANBUS_SPEED = 500000;
const uint8_t MY_CAN_ID = 5;

FlexCAN CANbus(500000);
elapsedMillis em;

void init()
{
    // Enable CAN transceiver
    pinMode(23, OUTPUT);
    digitalWrite(23, LOW);

    // Start CAN driver
    CANbus.begin();
}

void rx_broadcast(const CAN_message_t& msg)
{
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

void rx_command(const CAN_message_t& msg)
{
    msg_header_t header;
    header.decode(msg.id);
    // if (msg_type == MSG_TYPE::MSG_XTND)
    // {
    //     msg_type = (MSG_TYPE)msg.buf[0];
    // }
    switch (header.msg_type)
    {
    case MSG_TYPE::MSG_CMD:
        Serial.println("MSG_CMD");
        break;
    case MSG_TYPE::MSG_REQ:
        Serial.print("MSG_REQ");
        Serial.println(msg.buf[2] & 0x0f); // print length of request
        if (header.to_id == MY_CAN_ID)
        {
            msg_header_t header =
                {
                    .table = msg.buf[0],
                    .to_id = header.from_id,
                    .from_id = MY_CAN_ID,
                    .msg_type = MSG_TYPE::MSG_RSP,
                    .offset = (uint16_t)((msg.buf[2] >> 5) | ((uint16_t)msg.buf[1] << 3)),
                };
            CAN_message_t rsp =
                {
                    .id = header.pack(),
                    .ext = 1,
                    .len = (uint8_t)(msg.buf[2] & 0x0f),
                    .timeout = 0,
                    .buf = {0, 0, 0, 0, 0, 0, 0, 0},
                };

            CANbus.write(rsp);
        }
        break;
    case MSG_TYPE::MSG_RSP:
        Serial.println("MSG_RSP");
        break;
    case MSG_TYPE::MSG_XSUB:
        Serial.println("MSG_XSUB");
        break;
    case MSG_TYPE::MSG_BURN:
        Serial.println("MSG_BURN");
        break;
    case MSG_TYPE::OUTMSG_REQ:
        Serial.println("OUTMSG_REQ");
        break;
    case MSG_TYPE::OUTMSG_RSP:
        Serial.println("OUTMSG_RSP");
        break;
    case MSG_TYPE::MSG_XTND:
        Serial.println("MSG_XTND");
        break;
    default:
        Serial.println("UNDEF");
        break;
    }
}

void update()
{
    for (int i = CANbus.available(); i > 0; i--)
    {
        CAN_message_t msg;
        CANbus.read(msg);
        msg.ext ? rx_command(msg) : rx_broadcast(msg);
    }
    GV.connected = (em < CANBUS_TIMEOUT); // Timeout after 5s
}
} // namespace CanBus