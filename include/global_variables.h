#if !defined(GLOBAL_VARIABLES_H)
#define GLOBAL_VARIABLES_H

#include "stdint.h"

struct GVStruct
{
    bool alert;
    int16_t rpm;
    uint8_t current_gear;
    float acc_x;
    float acc_y;
    float acc_z;
    float acc_s;
};

extern GVStruct GV;

#endif // GLOBAL_VARIABLES_H