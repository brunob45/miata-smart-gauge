#include "canbus.h"

#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <filter_window.h>

#include "global.h"

namespace
{
const uint32_t CANBUS_TIMEOUT = 2000;
const uint32_t CANBUS_SPEED = 500000;
const uint8_t MY_CAN_ID = 5;

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CANbus;
bool wasConnected = false;
elapsedMillis last_frame = CANBUS_TIMEOUT;
bool initDone = false;
uint8_t initStep = 0;
uint8_t initIndex = 0;
bool requestPending = false;
uint32_t last_burn = 0;

const float RATIO_BIN = 0.25f; // [0, 0.50]
bool afrIsValid = false, afrWasValid = false;
uint32_t afrTimeValid = 0;
uint16_t last_pw = 0;
int16_t last_map = 0;

FilterWindow<int16_t, 10> filter_batv;
FilterWindow<int16_t, 10> filter_adc7;

inline int16_t F_TO_C(int16_t x)
{
    return 5.0f * ((x)-320) / 9.0f;
}

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
        offset = (can_id >> 18) & 0x3ff;
    }

    CAN_message_t get_msg()
    {
        CAN_message_t msg;
        msg.id = 0;

        msg.id |= ((uint32_t)table & 0x10) >> 2;
        msg.id |= ((uint32_t)table & 0x0f) << 3;
        msg.id |= ((uint32_t)to_id & 0x0f) << 7;
        msg.id |= ((uint32_t)from_id & 0x0f) << 11;
        msg.id |= ((uint32_t)msg_type & 0x07) << 15;
        msg.id |= ((uint32_t)offset & 0x3ff) << 18;

        msg.flags.extended = 1;
        return msg;
    }

    void print(Print& s)
    {
        s.print('{');
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

void send_value(uint8_t id,
                uint8_t table,
                uint16_t offset,
                uint8_t value)
{
    msg_header_t header{
        .table = table,
        .to_id = id,
        .from_id = MY_CAN_ID,
        .msg_type = MSG_TYPE::MSG_CMD,
        .offset = offset,
    };

    CAN_message_t msg = header.get_msg();
    msg.len = 1;
    msg.buf[0] = value;

    CANbus.write(msg);
}

void send_request(uint8_t id,
                  uint8_t table,
                  uint16_t offset,
                  uint8_t length)
{
    msg_header_t header{
        .table = table,
        .to_id = id,
        .from_id = MY_CAN_ID,
        .msg_type = MSG_TYPE::MSG_REQ,
        .offset = offset,
    };

    CAN_message_t msg = header.get_msg();
    msg.len = 3;
    msg.buf[0] = table;
    msg.buf[1] = uint8_t(offset >> 3);
    msg.buf[2] = uint8_t(((offset << 5) & 0xE0) | length);

    CANbus.write(msg);
    requestPending = true;
}

void send_burn(uint8_t id,
               uint8_t table)
{
    msg_header_t header{
        .table = table,
        .to_id = id,
        .from_id = MY_CAN_ID,
        .msg_type = MSG_TYPE::MSG_BURN,
        .offset = 0,
    };

    CAN_message_t msg = header.get_msg();
    msg.len = 0;

    CANbus.write(msg);
}

void update(int x, int y, float error, bool execute)
{
    if (execute)
    {
        const int index = x + y * 16;
        float ve = GV.ms.vetable[index];
        if (error < 0.96)
        {
            // too much fuel, decrease VE
            ve = max(ve - 0.05, 85);
            GV.ltt.err[index] = EGOERR::RICH;
        }
        else if (error > 1.02)
        {
            // too little fuel, increase VE
            ve = min(ve + 0.1, 115);
            GV.ltt.err[index] = EGOERR::LEAN;
        }
        else
        {
            GV.ltt.err[index] = EGOERR::OK;
        }
        if (roundl(GV.ms.vetable[index]) != roundl(ve))
        {
            send_value(0, 9, 256 + index, roundl(ve));
            GV.ltt.needBurn = true;
        }
        GV.ms.vetable[index] = ve;
    }
}

void updateLongTermTrim()
{
    const bool accel = GV.ms.pw1 > (last_pw + 700);
    last_pw = GV.ms.pw1;

    const bool decel = GV.ms.map < (last_map - 50);
    last_map = GV.ms.map;

    GV.ltt.accelDetected = accel || decel;

    int x, y, x2, y2;
    for (x = 1; x < 15; x++)
    {
        if (GV.ms.rpm <= GV.ms.rpm_table[x])
            break;
    }
    {
        float rpm1 = GV.ms.rpm_table[x - 1];
        float rpm2 = GV.ms.rpm_table[x];
        float ax = (GV.ms.rpm - rpm1) / (rpm2 - rpm1);
        if (ax <= RATIO_BIN)
        {
            // value near rpm1
            x = x - 1;
            x2 = x;
        }
        else if (ax >= (1 - RATIO_BIN))
        {
            // value near rpm2
            x2 = x;
        }
        else
        {
            // value in the middle
            x2 = x;
            x = x - 1;
        }
    }

    for (y = 1; y < 15; y++)
    {
        if (GV.ms.map <= GV.ms.map_table[y])
            break;
    }
    {
        float map1 = GV.ms.map_table[y - 1];
        float map2 = GV.ms.map_table[y];
        float ay = (GV.ms.map - map1) / (map2 - map1);
        if (ay <= RATIO_BIN)
        {
            y = y - 1;
            y2 = y;
        }
        else if (ay >= (1 - RATIO_BIN))
        {
            y2 = y;
        }
        else
        {
            y2 = y;
            y = y - 1;
        }
    }
    GV.ltt.x[0] = x;
    GV.ltt.x[1] = x2;
    GV.ltt.y[0] = y;
    GV.ltt.y[1] = y2;

    if ((GV.ltt.needBurn) &&
        (GV.ms.pw1 == 0) &&                // decel fuel cut, avoid jerk
        (GV.ms.rpm > 2500) &&              // high rpm to avoid stall
        ((millis() - last_burn) > 600000)) // 10 minutes
    {
        send_burn(0, 9);
        GV.ltt.needBurn = false;
        last_burn = millis();
    }

    afrIsValid = (GV.ms.pw1 > 0) &&
                 (GV.ms.afr1 > 100) && // 10.0 afr
                 (GV.ms.rpm > 500) && // 500 rpm
                 (GV.ms.clt > 650) && // 65.0 C
                 (!GV.ltt.accelDetected);

    if (afrIsValid && !afrWasValid)
    {
        afrTimeValid = millis();
    }
    afrWasValid = afrIsValid;
    //GV.ltt.engaged = afrIsValid && (millis() - afrTimeValid) > 2000;

    if (GV.ms.afrtgt1 > 0)
    {
        GV.ltt.error = GV.ms.egocor1 / 1000.0f * GV.ms.afr1 / GV.ms.afrtgt1;
    }
    else
    {
        GV.ltt.error = 1.0f;
    }

    if (GV.ltt.engaged)
    {
        update(x, y, GV.ltt.error, true);
        update(x2, y, GV.ltt.error, x != x2);
        update(x, y2, GV.ltt.error, y != y2);
        update(x2, y2, GV.ltt.error, x != x2 && y != y2);
    }
}
} // namespace

namespace CanBus
{
void init()
{
    // Enable CAN transceiver
    pinMode(23, OUTPUT);
    digitalWrite(23, LOW);

    // Start CAN driver
    CANbus.begin();
    CANbus.setBaudRate(500000);
}

bool rx_broadcast(const CAN_message_t& msg)
{
    const int BASE_ID = 1520;
    // Convert each field from big endian to local format
    switch (msg.id - BASE_ID)
    {
    case 0:
        GV.ms.seconds = (msg.buf[0] << 8) | (msg.buf[1] << 0);
        GV.ms.pw1 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
        GV.ms.pw2 = (msg.buf[4] << 8) | (msg.buf[5] << 0);
        GV.ms.rpm = (msg.buf[6] << 8) | (msg.buf[7] << 0);
        break;
    case 1:
        GV.ms.adv = (msg.buf[0] << 8) | (msg.buf[1] << 0);
        GV.ms.squirt = msg.buf[2];
        GV.ms.engine = msg.buf[3];
        GV.ms.afrtgt1 = msg.buf[4];
        GV.ms.afrtgt2 = msg.buf[5];
        // 6-7 unused
        break;
    case 2:
        GV.ms.baro = (msg.buf[0] << 8) | (msg.buf[1] << 0);
        GV.ms.map = (msg.buf[2] << 8) | (msg.buf[3] << 0);
        GV.ms.mat = F_TO_C((msg.buf[4] << 8) | (msg.buf[5] << 0));
        GV.ms.clt = F_TO_C((msg.buf[6] << 8) | (msg.buf[7] << 0));
        break;
    case 3:
        GV.ms.tps = (msg.buf[0] << 8) | (msg.buf[1] << 0);
        GV.ms.batt = filter_batv((msg.buf[2] << 8) | (msg.buf[3] << 0));
        GV.ms.afr1 = (msg.buf[4] << 8) | (msg.buf[5] << 0);
        GV.ms.afr2 = (msg.buf[6] << 8) | (msg.buf[7] << 0);
        break;
    case 4:
        GV.ms.knock = (msg.buf[0] << 8) | (msg.buf[1] << 0);
        GV.ms.egocor1 = (msg.buf[2] << 8) | (msg.buf[3] << 0);
        GV.ms.egocor2 = (msg.buf[4] << 8) | (msg.buf[5] << 0);
        GV.ms.aircor = (msg.buf[6] << 8) | (msg.buf[7] << 0);
        break;
    case 15:
        GV.ms.sensors9 = (msg.buf[0] << 8) | (msg.buf[1] << 0); // adc6
        GV.ms.sensors10 = filter_adc7((msg.buf[2] << 8) | (msg.buf[3] << 0)); // adc7
        // 4-7 unused
    case 59:
        // 0-5 unused
        GV.ms.deadtime1 = (msg.buf[6] << 8) | (msg.buf[7] << 0);
        updateLongTermTrim();
        break;
    default:
        // unknown id
        return false;
    }
    return true;
}

bool rx_command(const CAN_message_t& msg)
{
    msg_header_t header;
    header.decode(msg.id);
    if (header.msg_type == MSG_TYPE::MSG_XTND)
    {
        header.msg_type = (MSG_TYPE)msg.buf[0];
    }

    if (header.to_id != MY_CAN_ID)
    {
        return false;
    }

    switch (header.msg_type)
    {
    case MSG_TYPE::MSG_REQ:
    {
        msg_header_t rsp_header =
            {
                .table = msg.buf[0],
                .to_id = header.from_id,
                .from_id = MY_CAN_ID,
                .msg_type = MSG_TYPE::MSG_RSP,
                .offset = (uint16_t)((msg.buf[2] >> 5) | ((uint16_t)msg.buf[1] << 3)),
            };

        CAN_message_t rsp = rsp_header.get_msg();
        rsp.len = (uint8_t)(msg.buf[2] & 0x0f);

        if (header.table == 7)
        {
            if (header.offset == 2)
            {
                rsp.buf[0] = uint8_t(GV.fault_code >> 8);
                rsp.buf[1] = uint8_t(GV.fault_code >> 0);
                rsp.buf[2] = uint8_t(int16_t(GV.accel.x * 100) >> 8);
                rsp.buf[3] = uint8_t(int16_t(GV.accel.x * 100) >> 0);
                rsp.buf[4] = uint8_t(int16_t(GV.accel.y * 100) >> 8);
                rsp.buf[5] = uint8_t(int16_t(GV.accel.y * 100) >> 0);
                rsp.buf[6] = uint8_t(int16_t(GV.accel.z * 100) >> 8);
                rsp.buf[7] = uint8_t(int16_t(GV.accel.z * 100) >> 0);
            }
            else if (header.offset == 10)
            {
                rsp.buf[0] = uint8_t(GV.vss >> 8);
                rsp.buf[1] = uint8_t(GV.vss >> 0);
            }
        }
        // else send 0

        CANbus.write(rsp);
        break;
    }
    case MSG_TYPE::MSG_RSP:
    case MSG_TYPE::MSG_CMD:
    {
        // Serial.print("MSG_CMD ");
        // Serial.println(msg.buf[0]);
        requestPending = false;

        if (header.table == 9 && header.offset >= 256 && header.offset <= (512 - msg.len))
        {
            for (int i = 0; i < msg.len; i++)
            {
                GV.ms.vetable[(header.offset - 256) + i] = msg.buf[i];
            }
        }
        else if (header.table == 9 && header.offset >= 800 && header.offset <= (832 - msg.len))
        {
            for (int i = 0; i < msg.len / 2; i++)
            {
                GV.ms.rpm_table[(header.offset - 800) / 2 + i] = (uint16_t)msg.buf[i * 2 + 1] + (msg.buf[i * 2] << 8);
            }
        }
        else if (header.table == 9 && header.offset >= 896 && header.offset <= (928 - msg.len))
        {
            for (int i = 0; i < msg.len / 2; i++)
            {
                GV.ms.map_table[(header.offset - 896) / 2 + i] = (uint16_t)msg.buf[i * 2 + 1] + (msg.buf[i * 2] << 8);
            }
        }
        break;
    }
    case MSG_TYPE::MSG_XSUB:
        break;
    case MSG_TYPE::MSG_BURN:
        break;
    case MSG_TYPE::OUTMSG_REQ:
        break;
    case MSG_TYPE::OUTMSG_RSP:
        break;
    case MSG_TYPE::MSG_BURNACK:
        break;
    default:
        break;
    }
    return true;
}

void update()
{
    CAN_message_t msg;
    while (CANbus.read(msg))
    {
        if (msg.flags.extended ? rx_command(msg) : rx_broadcast(msg))
        {
            last_frame = 0;
        }
    }

    GV.connected = (last_frame < CANBUS_TIMEOUT); // Timeout after 5s
    if (wasConnected && !GV.connected)
    {
        // Connection lost, reset ms values to 0
        for (int i = sizeof(GV.ms); i > 0; i--)
        {
            ((uint8_t*)&GV.ms)[i] = 0;
        }
        initDone = false;
    }
    wasConnected = GV.connected;

    // Read ve table #2 content
    if (GV.connected && !initDone && !requestPending)
    {
        switch (initStep)
        {
        case 0:
            send_request(0, 9, 256 + (initIndex * 8), 8);
            if (++initIndex >= (256 / 8)) // 256 bytes, 8 bytes per msg
            {
                initIndex = 0;
                ++initStep;
            }
            break;
        case 1:
            send_request(0, 9, 800 + (initIndex * 8), 8);
            if (++initIndex >= (16 / 4)) // 16 int, 4 int per msg
            {
                initIndex = 0;
                ++initStep;
            }
            break;
        case 2:
            send_request(0, 9, 896 + (initIndex * 8), 8);
            if (++initIndex >= (16 / 4)) // 16 int, 4 int per msg
            {
                initIndex = 0;
                ++initStep;
            }
            break;
        case 3:
            initDone = true;
            break;
        }
    }
}
} // namespace CanBus