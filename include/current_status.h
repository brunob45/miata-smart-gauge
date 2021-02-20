#if !defined(CURRENT_STATUS_H)
#define CURRENT_STATUS_H

#include <stdint.h>

struct CurrentStatus
{
    struct
    {
        bool ready : 1;
        bool crank : 1;
        bool ase : 1;
        bool wue : 1;
        bool tpsaen : 1;
        bool tpsden : 1;
        bool mapaen : 1;
        bool mapden : 1;
    } engine;
    int16_t rpm;
    uint16_t engine_load;
    uint16_t inj1ActualPw;
    uint16_t inj2ActualPw;
    struct
    {
        bool full_sync : 1;
        bool half_sync : 1;
        bool need_burn : 1;
        bool idle : 1;
        bool fan : 1;
        bool run : 1;
        bool soft_limit : 1;
        bool hard_limit : 1;
    } status1;
    int16_t teeth_per_event;
    int16_t crank_angle_error;
    uint16_t base_pw;
    uint16_t debug;
    uint16_t pw;
    int16_t advance;
    uint16_t dwell;
    uint16_t open_time;
    uint8_t n_squirts;
    uint16_t brv_adc;
    uint16_t map_adc;
    uint16_t mat_adc;
    uint16_t clt_adc;
    uint16_t tps_adc;
    uint16_t ego_adc;
    uint16_t adc6;
    uint16_t adc7;
    uint16_t brv;
    uint16_t map;
    uint8_t tps;
    uint16_t ign1ActualPw;
    uint16_t ign2ActualPw;
    uint16_t loop_speed;
    int16_t clt;
    int16_t mat;
    uint16_t inj1Period;
    uint16_t inj2Period;
    uint16_t ign1Period;
    uint16_t ign2Period;
    uint16_t seconds;
    struct
    {
        bool fp_on : 1;
        bool fp_primed : 1;
        bool decel_fuel_cut : 1;
        bool clutch : 1;
        bool launch_arm : 1;
        bool launch_active : 1;
        bool flood_clear : 1;
        bool ego_ready : 1;
    } status2;
    uint16_t enrich_warmup;
    uint16_t enrich_ase;
    uint16_t idle_target;
    uint16_t total_cor;
    uint8_t limiter_level_inj;
    uint8_t limiter_level_ign;
    uint16_t ego_tgt;
    uint16_t ego;
    int16_t mapdot;
    uint16_t enrich_accel;
    uint8_t accel_timeout;
    uint16_t stack_pointer;
    uint16_t airden_cor;
    uint16_t cranking_pw;
    uint16_t vss_delta;
    uint16_t priming_pulse;
    uint16_t egocor;
} __attribute__((packed));

#endif // CURRENT_STATUS_H
