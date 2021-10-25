#include "speedo.h"

#include <FreqMeasureMulti.h>
#include <elapsedMillis.h>

#include "filter.h"
#include "global.h"

namespace Speedo
{
namespace
{
volatile uint32_t last_edge_period;
volatile uint32_t last_edge_time;
volatile bool have_sync = false;

// filter constant = "refresh rate" / "signal lag" (in seconds)
FilterClass filter(0.05f / 0.2f);

} // namespace

void pin_callback()
{
    const uint32_t now = micros();
    if (have_sync)
    {
        last_edge_period = now - last_edge_time;
    }
    last_edge_time = now;
    have_sync = true;
}

void init()
{
    have_sync = false;
    attachInterrupt(21, pin_callback, RISING);
}

void update()
{
    static elapsedMillis last_update;
    if (last_update < 50)
        return;

    if (have_sync)
    {
        const uint32_t my_period = max(last_edge_period, micros() - last_edge_time);
        if (my_period > 500000) // 500 ms = 2 Hz
        {
            have_sync = false;
            GV.vss = 0;
            filter.reset();
        }
        else
        {
            GV.vss = uint16_t(filter.put(my_period) / 10);
        }
    }
    last_update = 0;
}

} // namespace Speedo