#if !defined(DASH_GLOBAL_H)
#define DASH_GLOBAL_H

class PeriodicTimer
{
    systime_t prev;

public:
    PeriodicTimer()
    {
        init();
    }

    void init()
    {
        prev = chVTGetSystemTime();
    }

    void sleep(uint ms)
    {
        prev = chThdSleepUntilWindowed(prev, chTimeAddX(prev, TIME_MS2I(ms)));
    }
};

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
        size_t stack;
    } ms;

    struct
    {
        float x;
        float y;
        float z;
        size_t stack;
    } accel;

    struct
    {
        uint16_t frpm[16];
        uint16_t fmap[16];
        uint8_t values[16 * 16];
    } ftrim;

    struct
    {
        bool fix;
        float spd;
        float dir;
        uint16_t tod;
        size_t stack;
    } gps;

    bool alert;
    uint8_t gear;
    uint16_t lumi;
    bool connected;
    uint16_t vss;
    uint16_t fault_code = 0x7fff;
};

#endif // DASH_GLOBAL_H