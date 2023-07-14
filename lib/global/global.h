#if !defined(DASH_GLOBAL_H)
#define DASH_GLOBAL_H

enum class EGOERR: uint8_t
{
    UNKNOWN = 0,
    LEAN = 1,
    RICH = 2,
    OK = 3
};

struct GlobalVars
{
    struct
    {
        // 0
        uint16_t seconds;   // 1 s
        uint16_t pw1;       // 0.001 ms
        uint16_t pw2;       // 0.001 ms
        uint16_t rpm;       // 1 rpm
        // 1
        int16_t adv;        // 0.1 deg
        uint8_t squirt;
        uint8_t engine;
        uint8_t afrtgt1;    // 0.1 AFR
        uint8_t afrtgt2;    // 0.1 AFR
        // 2
        int16_t baro;       // 0.1 kPa
        int16_t map;        // 0.1 kPa
        int16_t mat;        // 0.1 deg C
        int16_t clt;        // 0.1 deg C
        // 3
        int16_t tps;        // 0.1 %
        int16_t batt;       // 0.1 V
        int16_t afr1;       // 0.1 AFR
        int16_t afr2;       // 0.1 AFR
        // 4
        int16_t knock;      // 0.1 %
        int16_t egocor1;    // 0.1 %
        int16_t egocor2;    // 0.1 %
        int16_t aircor;     // 0.1 %
        // 15
        int16_t sensors9;
        int16_t sensors10;
        // 59
        int16_t deadtime1;  // 0.001 ms

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
        EGOERR err[16 * 16];
        bool needBurn;
    } ltt;

    struct
    {
        float x;
        float y;
        float z;
    } accel;

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