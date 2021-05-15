#include "speedo.h"

#include <FreqMeasureMulti.h>
#include <elapsedMillis.h>

#include "filter.h"

namespace Speedo
{
namespace
{
FreqMeasureMulti fmm;
elapsedMillis time_since_last_edge;
uint32_t period_max;
uint32_t period_min;
uint32_t last_period;

FilterClass filter(0.1f);

} // namespace

void init()
{
    fmm.begin(21);
    time_since_last_edge = 0;
    period_max = fmm.countToFrequency(1);   // 30 rpm = 1 Hz
    period_min = fmm.countToFrequency(333); // 10000 rpm = 333.3 Hz
    last_period = 0;

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

        // filter noise - reject pulses that are too fast
        last_period += fmm.read();
        if (last_period >= period_min)
        {
            filter.put(last_period);
            last_period = 0;
        }
    }
    if (time_since_last_edge > 250) // 250ms = 4Hz = 120rpm
    {
        time_since_last_edge = 0;
        filter.put(period_max * 30);
    }
}

uint16_t get_value()
{
    const uint32_t period = filter.get();
    float freq = 0;
    if (period < period_max)
    {
        freq = fmm.countToFrequency(period) * 30.0f; // 1 Hz = 30 rpm
        freq = constrain(freq, 0, 9999);
    }
    return freq;
}

} // namespace Speedo