#include "speedo.h"

#include <FreqMeasureMulti.h>
#include <elapsedMillis.h>

#include "filter.h"
#include "global.h"

namespace Speedo
{
namespace
{
FreqMeasureMulti fmm;
elapsedMillis time_since_last_edge;
bool lost_sync = false;

FilterClass filter(0.1f);

} // namespace

void init()
{
    fmm.begin(21);
    time_since_last_edge = 0;
    lost_sync = true;

    // clear fmm buffer
    while (fmm.available())
    {
        fmm.read();
    }
}

void update()
{
    for (int i = fmm.available(); i > 0; i--)
    {
        time_since_last_edge = 0;
        const uint32_t period = fmm.read();

        if (lost_sync)
        {
            lost_sync = false;
        }
        else
        {
            filter.put(period);
        }
    }

    if (time_since_last_edge > 500) // 500 ms = 2 Hz
    {
        time_since_last_edge = 0;
        filter.put(fmm.countToFrequency(2)); // 2 Hz
        lost_sync = true;
    }

    const float frequency = fmm.countToFrequency(filter.get());
    GV.vss = lost_sync
                 ? 0
                 : constrain(frequency, 0, 65535);
}

} // namespace Speedo