#include "speedo.h"

#include "fast_micros.h"
#include "filter.h"
#include "global.h"

namespace Speedo
{
namespace
{
const uint8_t PIN_VSS = 22;
volatile uint32_t v_edge_cnt = 0;
uint32_t last_edge_time;

// filter constant = "refresh rate" / "signal lag" (in seconds)
FilterClass filter(0.1f / 0.5f);

} // namespace

void pin_callback()
{
    bool do_sample = true;
    for (int i = 0; i < 8; i++)
    {
        do_sample &= digitalReadFast(PIN_VSS);
    }
    if (do_sample)
    {
        v_edge_cnt++;
    }
}

void init()
{
    last_edge_time = fast_micros();
    pinMode(PIN_VSS, INPUT);
    attachInterrupt(PIN_VSS, pin_callback, RISING);
}

void update()
{
    const uint32_t now = fast_micros();
    const uint32_t my_period = now - last_edge_time;

    if (my_period < 100'000)
        return;

    noInterrupts();
    const uint32_t edge_cnt = v_edge_cnt;
    v_edge_cnt = 0;
    interrupts();

    GV.vss = uint16_t(filter.put(1.5f * 1'000'000.0f * edge_cnt / my_period));
    last_edge_time = now;
}

} // namespace Speedo