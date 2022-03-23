#include "speedo.h"

#include <Arduino.h>
#include <ChRt.h>

#include "fast_micros.h"
#include "filter.h"
#include "global.h"

namespace Speedo
{
namespace
{
THD_WORKING_AREA(waThd, 2 * 256);

volatile uint32_t last_edge_period;
volatile uint32_t last_edge_time;
volatile bool have_sync = false;
} // namespace

void pin_callback()
{
    const uint32_t now = millis();
    if (have_sync)
    {
        last_edge_period = now - last_edge_time;
    }
    last_edge_time = now;
    have_sync = true;
}

THD_FUNCTION(ThreadSpeedo, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    // filter constant = "refresh rate" / "signal lag" (in seconds)
    FilterClass filter(0.05f / 0.2f);
    elapsedMillis last_update = 0;

    attachInterrupt(21, pin_callback, RISING);

    for (;;)
    {
        chThdSleepMilliseconds(50);

        if (have_sync)
        {
            const uint32_t my_period = max(last_edge_period, millis() - last_edge_time);
            if (my_period > 500) // 500 ms = 2 Hz
            {
                last_edge_period = -1;
                have_sync = false;
                pGV->vss = -1;
                filter.reset();
            }
            else
            {
                pGV->vss = uint16_t(filter.put(my_period) / 10);
            }
        }
        last_update = 0;
    }
}

void initThreads(tprio_t prio, void* arg)
{
    chThdCreateStatic(waThd, sizeof(waThd), prio, ThreadSpeedo, arg);
}
size_t getUnusedStack()
{
    return chUnusedThreadStack(waThd, sizeof(waThd));
}

} // namespace Speedo