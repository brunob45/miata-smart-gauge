#include "can_ll.h"

#include <FlexCAN_T4.h>

namespace CAN_LL
{
struct rx_mask_t {
    uint32_t id;
    uint32_t mask;
    rx_callback_t callback;
};

const uint32_t CANBUS_TIMEOUT = 2000;
static bool doInit = true;
static FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CANbus;

static rx_mask_t rx_masks[32];
static uint8_t rx_mask_cnt = 0;

static void setup()
{
    if (doInit)
    {
        // Enable CAN transceiver
        pinMode(23, OUTPUT);
        digitalWrite(23, LOW);

        // Start CAN driver
        CANbus.begin();
        CANbus.setBaudRate(CANBUS_SPEED);
        doInit = false;
    }
}

void send(uint32_t aId, uint8_t aLen, uint8_t aBuf[8])
{
    setup();

    CAN_message_t msg;
    msg.id = aId;
    msg.flags.extended = 1;
    msg.len = aLen;
    for(int i = 0; i < aLen; i++)
    {
        msg.buf[i] = aBuf[i];
    }
    CANbus.write(msg);
}

void attach(uint32_t aId, uint32_t aMask, rx_callback_t aCallback)
{
    setup();

    if (rx_mask_cnt < 255)
    {
        rx_masks[rx_mask_cnt].id = aId;
        rx_masks[rx_mask_cnt].mask = aMask;
        rx_masks[rx_mask_cnt].callback = aCallback;
        rx_mask_cnt++;
    }
}

void attach(uint32_t aId, rx_callback_t aCallback)
{
    attach(aId, 0xffff, aCallback);
}

bool update()
{
    static elapsedMillis em = CANBUS_TIMEOUT;

    setup();

    CAN_message_t msg;
    while(CANbus.read(msg))
    {
        for (uint i = 0; i < rx_mask_cnt; i++)
        {
            if (rx_masks[i].id == (msg.id & rx_masks[i].mask))
            {
                rx_masks[i].callback(msg.id, msg.buf);
                em = 0;
            }
        }
    }
    return (em < CANBUS_TIMEOUT); // Timeout after 5s
}
}