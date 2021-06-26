#include "accel.h"

#include <Adafruit_MSA301.h>

#include "filter.h"
#include "global.h"

namespace Accel
{
namespace
{
Adafruit_MSA301 msa;
FilterClass fx(0.05f), fy(0.05f), fz(0.05f);

const float ANGLE = 24.0f * DEG_TO_RAD;
const float SINUS = sinf(ANGLE);
const float COSINUS = cosf(ANGLE);
} // namespace

float AccelValue::norm()
{
    float n = x * x;
    n += y * y;
    n += z * z;
    return sqrtf(n);
}

void init(void)
{
    msa.begin();
}

void update(void)
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update < 20)
        return;

    msa.read();

    fx.put(msa.x_g);

    // x2 = cosβ*x1 − sinβ*y1
    fy.put(COSINUS * msa.z_g - SINUS * msa.y_g);

    // y2 = sinβ*x1 + cosβ*y1
    fz.put(SINUS * msa.z_g + COSINUS * msa.y_g);

    GV.accel.x = fx.get();
    GV.accel.y = fy.get();
    GV.accel.z = fz.get();

    last_update = now;
}

void print_debug(Print& p)
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

    //a.setPowerMode(MSA301_SUSPENDMODE);
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

    //a.setBandwidth(MSA301_BANDWIDTH_31_25_HZ);
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

    //a.setRange(MSA301_RANGE_2_G);
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

    //a.setResolution(MSA301_RESOLUTION_14 );
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

AccelValue get()
{
    return AccelValue{fx.get(), fy.get(), fz.get()};
}

} // namespace Accel