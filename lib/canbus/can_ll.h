#ifndef CAN_LL_H
#define CAN_LL_H

#include <Arduino.h>

#ifndef CANBUS_SPEED
#define CANBUS_SPEED (500000UL)
#endif

namespace CAN_LL
{
typedef void (*rx_callback_t)(uint32_t id, uint8_t buf[8]);

void send(uint32_t aId, uint8_t aLen, uint8_t aBuf[8]);
void attach(uint32_t aId, rx_callback_t aCallback);
void attach(uint32_t aId, uint32_t aMask, rx_callback_t aCallback);
bool update();
}

#endif // CAN_LL_H