#include "accel.h"

#include <Adafruit_MSA301.h>

#include "filter.h"
#include "global.h"

namespace Accel
{
namespace
{
THD_WORKING_AREA(waThdAccel, 3 * 256);

void accelUpdate(Adafruit_MSA301* msa)
{
    if (msa)
    {
        msa->read();
        msa->x_g = (msa->x + 45) / 2048.0f;
        msa->y_g = (msa->y + 0) / 2048.0f;
        msa->z_g = (msa->z + 180) / 2048.0f;
    }
}

THD_FUNCTION(ThreadAccel, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    Adafruit_MSA301 msa;

    // filter constant = "refresh rate" / "signal lag" (in seconds)
    FilterClass fx(0.020f / 0.40f), fy(0.020f / 0.40f), fz(0.020f / 0.40f);
    FilterClass fa(0.020f / 10.0f);

    // Init MSA301
    msa.begin();

    // Read accel values
    accelUpdate(&msa);

    // Init filters to first reading
    if (abs(msa.y_g) > 0.001f)
    {
        fa.reset(atanf(msa.z_g / msa.y_g));
    }
    else
    {
        fa.reset(15 * DEG_TO_RAD);
    }

    for (;;)
    {
        // Update accel values
        accelUpdate(&msa);

        float x = fx.put(msa.x_g);
        float y = fy.put(msa.y_g);
        float z = fz.put(msa.z_g);

        if (abs(y) > 0.001f)
        {
            pGV->accel.angle = fa.put(atanf(z / y));
        }

        float _cos = cosf(fa.get());
        float _sin = sinf(fa.get());

        pGV->accel.x = x;
        pGV->accel.y = _sin * z + _cos * y;
        pGV->accel.z = _cos * z - _sin * y;

        pGV->accel.stack = chUnusedThreadStack(waThdAccel, sizeof(waThdAccel));
        chThdSleepMilliseconds(20);
    }
}

void print_debug(Adafruit_MSA301& msa, Print& p)
{
    // a.setDataRate(MSA301_DATARATE_125_HZ);
    p.print("Data rate set to: ");
    switch (msa.getDataRate())
    {
    case MSA301_DATARATE_1_HZ:
        p.println("1 Hz");
        break;
    case MSA301_DATARATE_1_95_HZ:
        p.println("1.95 Hz");
        break;
    case MSA301_DATARATE_3_9_HZ:
        p.println("3.9 Hz");
        break;
    case MSA301_DATARATE_7_81_HZ:
        p.println("7.81 Hz");
        break;
    case MSA301_DATARATE_15_63_HZ:
        p.println("15.63 Hz");
        break;
    case MSA301_DATARATE_31_25_HZ:
        p.println("31.25 Hz");
        break;
    case MSA301_DATARATE_62_5_HZ:
        p.println("62.5 Hz");
        break;
    case MSA301_DATARATE_125_HZ:
        p.println("125 Hz");
        break;
    case MSA301_DATARATE_250_HZ:
        p.println("250 Hz");
        break;
    case MSA301_DATARATE_500_HZ:
        p.println("500 Hz");
        break;
    case MSA301_DATARATE_1000_HZ:
        p.println("1000 Hz");
        break;
    }

    // a.setPowerMode(MSA301_SUSPENDMODE);
    p.print("Power mode set to: ");
    switch (msa.getPowerMode())
    {
    case MSA301_NORMALMODE:
        p.println("Normal");
        break;
    case MSA301_LOWPOWERMODE:
        p.println("Low Power");
        break;
    case MSA301_SUSPENDMODE:
        p.println("Suspend");
        break;
    }

    // a.setBandwidth(MSA301_BANDWIDTH_31_25_HZ);
    p.print("Bandwidth set to: ");
    switch (msa.getBandwidth())
    {
    case MSA301_BANDWIDTH_1_95_HZ:
        p.println("1.95 Hz");
        break;
    case MSA301_BANDWIDTH_3_9_HZ:
        p.println("3.9 Hz");
        break;
    case MSA301_BANDWIDTH_7_81_HZ:
        p.println("7.81 Hz");
        break;
    case MSA301_BANDWIDTH_15_63_HZ:
        p.println("15.63 Hz");
        break;
    case MSA301_BANDWIDTH_31_25_HZ:
        p.println("31.25 Hz");
        break;
    case MSA301_BANDWIDTH_62_5_HZ:
        p.println("62.5 Hz");
        break;
    case MSA301_BANDWIDTH_125_HZ:
        p.println("125 Hz");
        break;
    case MSA301_BANDWIDTH_250_HZ:
        p.println("250 Hz");
        break;
    case MSA301_BANDWIDTH_500_HZ:
        p.println("500 Hz");
        break;
    }

    // a.setRange(MSA301_RANGE_2_G);
    p.print("Range set to: ");
    switch (msa.getRange())
    {
    case MSA301_RANGE_2_G:
        p.println("+-2G");
        break;
    case MSA301_RANGE_4_G:
        p.println("+-4G");
        break;
    case MSA301_RANGE_8_G:
        p.println("+-8G");
        break;
    case MSA301_RANGE_16_G:
        p.println("+-16G");
        break;
    }

    // a.setResolution(MSA301_RESOLUTION_14 );
    p.print("Resolution set to: ");
    switch (msa.getResolution())
    {
    case MSA301_RESOLUTION_14:
        p.println("14 bits");
        break;
    case MSA301_RESOLUTION_12:
        p.println("12 bits");
        break;
    case MSA301_RESOLUTION_10:
        p.println("10 bits");
        break;
    case MSA301_RESOLUTION_8:
        p.println("8 bits");
        break;
    }
}
} // namespace

void initThreads(tprio_t prio, void* arg)
{
    chThdCreateStatic(waThdAccel, sizeof(waThdAccel), prio, ThreadAccel, arg);
}
} // namespace Accel