#ifndef CAN_HEADER_H
#define CAN_HEADER_H

#include <Arduino.h>

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

}

#endif // CAN_HEADER_H