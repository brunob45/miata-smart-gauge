#if !defined(DASH_GLOBAL_H)
#define DASH_GLOBAL_H

struct GlobalVars
{
    struct
    {
        // 1512
        int16_t map;
        uint16_t rpm;
        int16_t clt;
        int16_t tps;
        // 1513
        uint16_t pw1;
        uint16_t pw2;
        int16_t mat;
        int16_t adv;
        // 1514
        uint8_t afrtgt;
        uint8_t afr;
        int16_t egocor;
        int16_t egt;
        int16_t pwseq;
        // 1515
        int16_t batt;
        int16_t sensors1;
        int16_t sensors2;
        int16_t knk_rtd;

        float vetable[16 * 16];
        uint16_t rpm_table[16];
        int16_t map_table[16];
    } ms;

    struct
    {
        bool engaged;
        bool accelDetected;
        float error;
        int x[2], y[2];
        uint8_t err[16 * 16];
    } ltt;

    struct
    {
        float x;
        float y;
        float z;
    } accel;

    struct
    {
        uint16_t frpm[16];
        uint16_t fmap[16];
        uint8_t values[16 * 16];
    } ftrim;

    bool alert;
    uint8_t gear;
    uint16_t lumi;
    bool connected;
    uint16_t vss;
    uint16_t fault_code = 0x7fff;
    uint16_t waSize;
    float temperature;
};

extern GlobalVars GV;

#endif // DASH_GLOBAL_H