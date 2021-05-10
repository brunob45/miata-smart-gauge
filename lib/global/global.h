#if !defined(DASH_GLOBAL_H)
#define DASH_GLOBAL_H

struct GlobalVars
{
    struct
    {
        int16_t map;
        uint16_t rpm;
        int16_t clt;
        int16_t tps;

        uint16_t pw1;
        uint16_t pw2;
        int16_t mat;
        int16_t adv;

        uint8_t afrtgt;
        uint8_t afr;
        int16_t egocor;
        int16_t egt;
        int16_t pwseq;

        int16_t batt;
        int16_t sensors1;
        int16_t sensors2;
        int16_t knk_rtd;
    } ms;

    struct
    {
        float x;
        float y;
        float z;
    } accel;

    bool alert;
    uint8_t gear;
    uint16_t lumi;
};

extern GlobalVars GV;

#endif // DASH_GLOBAL_H