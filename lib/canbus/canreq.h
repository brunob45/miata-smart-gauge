#ifndef CAN_REQ_H
#define CAN_REQ_H

#include "canheader.h"

namespace CanBus
{

class CanReq {
public:
    bool isDone;
    bool isError;
    uint32_t output;
    uint8_t table;
    uint16_t offset;

private:
    msg_header_t tx_header, rx_header;

public:
    void operator()(uint8_t aTable, uint16_t aOffset)
    {

    }
};
}

#endif // CAN_REQ_H