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
    uint32_t id;
    uint8_t table;
    uint8_t to_id;
    uint8_t from_id;
    MSG_TYPE msg_type;
    uint16_t offset;

    void decode(uint32_t can_id)
    {
        id = can_id;

        table = (can_id >> 3) & 0x0f;
        table |= ((can_id << 2) & 0x10); // add last bit to table index

        to_id = (can_id >> 7) & 0x0f;
        from_id = (can_id >> 11) & 0x0f;
        msg_type = (MSG_TYPE)((can_id >> 15) & 0x07);
        offset = (can_id >> 18) & 0x3ff;
    }

    uint32_t pack()
    {
        id = 0;

        id |= ((uint32_t)table & 0x10) >> 2;
        id |= ((uint32_t)table & 0x0f) << 3;
        id |= ((uint32_t)to_id & 0x0f) << 7;
        id |= ((uint32_t)from_id & 0x0f) << 11;
        id |= ((uint32_t)msg_type & 0x07) << 15;
        id |= ((uint32_t)offset & 0x3ff) << 18;

        return id;
    }

    void print(Print& s)
    {
        s.print('{');
        s.print(id, HEX);
        s.print(',');
        s.print(table);
        s.print(',');
        s.print(to_id);
        s.print(',');
        s.print(from_id);
        s.print(',');

        switch (msg_type)
        {
        case MSG_TYPE::MSG_CMD:
            s.print("MSG_CMD");
            break;
        case MSG_TYPE::MSG_REQ:
            s.print("MSG_REQ");
            break;
        case MSG_TYPE::MSG_RSP:
            s.print("MSG_RSP");
            break;
        case MSG_TYPE::MSG_XSUB:
            s.print("MSG_XSUB");
            break;
        case MSG_TYPE::MSG_BURN:
            s.print("MSG_BURN");
            break;
        case MSG_TYPE::OUTMSG_REQ:
            s.print("OUTMSG_REQ");
            break;
        case MSG_TYPE::OUTMSG_RSP:
            s.print("OUTMSG_RSP");
            break;
        case MSG_TYPE::MSG_XTND:
            s.print("MSG_XTND");
            break;
        default:
            s.print("UNDEF");
            break;
        }

        s.print(',');
        s.print(offset);
        s.print('}');
    }

    void println(Print& s)
    {
        print(s);
        s.println();
    }
};

const uint32_t CANBUS_TIMEOUT = 2000;
const uint32_t CANBUS_SPEED = 500000;
const uint8_t MY_CAN_ID = 5;

static FlexCAN CANbus(500000);

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
    static elapsedMillis em;

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
    GV.connected = (em < CANBUS_TIMEOUT); // Timeout after 5s
}

void rx_command(const CAN_message_t& msg)
{
    msg_header_t header;
    header.decode(msg.id);

    // if (msg_type == MSG_TYPE::MSG_XTND)
    // {
    //     msg_type = (MSG_TYPE)msg.buf[0];
    // }

    // header.println(Serial);
    // for (int i = 0; i < msg.len; i++)
    // {
    //     Serial.print(msg.buf[i], HEX);
    //     Serial.print(',');
    // }
    // Serial.println();

    switch (header.msg_type)
    {
    case MSG_TYPE::MSG_CMD:
        break;
    case MSG_TYPE::MSG_REQ:
        if (header.to_id == MY_CAN_ID)
        {
            msg_header_t rsp_header =
                {
                    .id = 0,
                    .table = msg.buf[0],
                    .to_id = header.from_id,
                    .from_id = MY_CAN_ID,
                    .msg_type = MSG_TYPE::MSG_RSP,
                    .offset = (uint16_t)((msg.buf[2] >> 5) | ((uint16_t)msg.buf[1] << 3)),
                };
            CAN_message_t rsp =
                {
                    .id = rsp_header.pack(),
                    .ext = 1,
                    .len = (uint8_t)(msg.buf[2] & 0x0f),
                    .timeout = 0,
                    .buf = {
                        uint8_t(GV.vss >> 8),
                        uint8_t(GV.vss >> 0),
                        uint8_t(int16_t(GV.accel.x * 100) >> 8),
                        uint8_t(int16_t(GV.accel.x * 100) >> 0),
                        uint8_t(int16_t(GV.accel.y * 100) >> 8),
                        uint8_t(int16_t(GV.accel.y * 100) >> 0),
                        uint8_t(int16_t(GV.accel.z * 100) >> 8),
                        uint8_t(int16_t(GV.accel.z * 100) >> 0),
                    },
                };
            CANbus.write(rsp);
        }
        break;
    case MSG_TYPE::MSG_RSP:
        break;
    case MSG_TYPE::MSG_XSUB:
        break;
    case MSG_TYPE::MSG_BURN:
        break;
    case MSG_TYPE::OUTMSG_REQ:
        break;
    case MSG_TYPE::OUTMSG_RSP:
        break;
    case MSG_TYPE::MSG_XTND:
        break;
    default:
        break;
    }
}

void update()
{
    static bool wasConnected = false;

    for (int i = CANbus.available(); i > 0; i--)
    {
        CAN_message_t msg;
        CANbus.read(msg);
        msg.ext ? rx_command(msg) : rx_broadcast(msg);
    }

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