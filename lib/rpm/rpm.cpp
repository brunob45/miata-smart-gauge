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
elapsedMillis time_since_last_edge;

FilterClass<uint32_t, 8> filter;

} // namespace

void init()
{
    fmm.begin(21);
    time_since_last_edge = 0;

    // clear fmm buffer
    while (fmm.available())
    {
        fmm.read();
    }
}

void update()
{
    if (fmm.available())
    {
        time_since_last_edge = 0;
        filter.put(fmm.read());
    }
    else if (time_since_last_edge > 500) // 500ms = 2Hz = 120rpm
    {
        time_since_last_edge = 0;
        filter.put(0);
    }
}

uint16_t get_value()
{
    return constrain(fmm.countToFrequency(filter.get()), 0, 9999);
}

} // namespace RPM