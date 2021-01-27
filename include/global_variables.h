#if !defined(BB_GLOBAL_VARIABLES_H)
#define BB_GLOBAL_VARIABLES_H

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

#endif // BB_GLOBAL_VARIABLES_H