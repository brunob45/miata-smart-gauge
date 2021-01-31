#include "comm.h"

#include <Arduino.h>
#include <FastCRC.h>

#include "global_variables.h"

namespace Comm
{
const uint16_t SERIAL_WRAPPER_SIZE = 7;

FastCRC32 CRC32;

uint8_t rx_state = 0;
uint16_t rx_size = 0;
uint32_t rx_crc = 0;
char rx_command = '\0';

void init()
{
    Serial2.begin(115200);
    Serial.begin(115200);
}
void update()
{
    int rx_size = Serial.available();
    int tx_size = Serial2.availableForWrite();
    for (int xfer_size = min(rx_size, tx_size); xfer_size > 0; xfer_size--)
    {
        Serial2.write(Serial.read());
    }

    rx_size = Serial2.available();
    tx_size = Serial.availableForWrite();
    for (int xfer_size = min(rx_size, tx_size); xfer_size > 0; xfer_size--)
    {
        Serial.write(Serial2.read());
    }

    // static uint32_t last_update = 0;
    // const uint32_t now = millis();

    // if (Serial2.available() >= 2)
    // {
    //     Serial2.readBytes((uint8_t*)GV.rpm, 2);
    // }

    // if (now - last_update < 50)
    //     return;
    // last_update = now;

    // char tx_buffer[] = {0, 0, 'r', 0, 0, 1, 0, 2};
    // tx_buffer[1] = sizeof(tx_buffer);
    // Serial2.write(tx_buffer, sizeof(tx_buffer));
}
void request()
{
}
void send()
{
}
void receive()
{
    switch (rx_state)
    {
    case 0: // Receive request size
        if (Serial2.available() >= 2)
        {
            Serial.readBytes((char*)(&rx_size), 2);
            CRC32.crc32(nullptr, 0);
            rx_state = 1;
            break;
        }
    case 1: // Receive request command
        if (Serial2.available() >= 1)
        {
            rx_command = Serial2.read();
            CRC32.crc32_upd((uint8_t*)&rx_command, 1);
            rx_state = 2;
            break;
        }
    case 2: // Receive request data
        if (Serial2.available() >= 1)
        {

            if (rx_size-- <= 4)
            {
                rx_state = 3;
            }
            break;
        }
    case 3: // Receive request CRC
        if (Serial2.available() >= 4)
        {
            Serial2.readBytes((uint8_t*)&rx_crc, 4);
            break;
        }
    }
}

} // namespace Comm