#include "rpm.h"

#include <FreqMeasureMulti.h>
#include <elapsedMillis.h>

#include "filter.h"

namespace RPM
{
namespace
{
const uint8_t FILTER_SIZE = 4;

FreqMeasureMulti fmm;
uint32_t filter_values[FILTER_SIZE];
uint8_t filter_index;
elapsedMillis time_since_last_edge;

FilterClass<int32_t, 8> filter;

} // namespace

void init()
{
    fmm.begin(21, FREQMEASUREMULTI_INTERLEAVE);
    time_since_last_edge = 0;

    // init filter
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        filter_values[i] = 0;
    }
    filter_index = 0;

    // clear fmm buffer
    while (fmm.available())
    {
        fmm.read();
    }
}

void update()
{
    int n_avail = fmm.available();
    if (n_avail > 0)
    {
        time_since_last_edge = 0;

        for (int i = n_avail; i > 0; i--)
        {
            filter.put(fmm.countToFrequency(fmm.read()) * 60);
        }
    }
    else if (time_since_last_edge > 500) // 500ms = 2Hz = 120rpm
    {
        time_since_last_edge = 0;
        filter.put(0);
    }
}

uint16_t get_value()
{
    return filter.get();
}

} // namespace RPM