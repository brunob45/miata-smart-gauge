#if !defined(DASH_GLOBAL_H)
#define DASH_GLOBAL_H

struct GlobalVars
{
    int16_t rpm;
    bool alert;
    uint8_t gear;
    float accel[3];
    uint16_t lumi;
};

extern GlobalVars GV;

#endif // DASH_GLOBAL_H