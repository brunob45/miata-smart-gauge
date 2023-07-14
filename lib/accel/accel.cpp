#include "accel.h"

#include <Adafruit_MPU6050.h>

#include "filter.h"
#include "global.h"

namespace Accel
{
namespace
{
Adafruit_MPU6050 mpu;
sensors_event_t accel, gyro, temp;

// filter constant = "refresh rate" / "signal lag" (in seconds)
FilterClass fx(0.020f / 0.60f), fy(0.020f / 0.60f), fz(0.020f / 0.60f);
FilterClass gx(0.020f / 10.0f), gy(0.020f / 10.0f), gz(0.020f / 10.0f);
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
    // Init MSA301
    mpu.begin();

    // Read accel values
    mpu.getEvent(&accel, &gyro, &temp);

    // Init filters to first reading
    gx.reset(accel.acceleration.x);
    gy.reset(accel.acceleration.y);
    gz.reset(accel.acceleration.z);
}

void update(void)
{
    // Update accel values
    mpu.getEvent(&accel, &gyro, &temp);

    // Update gravity (low pass filter)
    // Update accel (band pass filter)
    GV.accel.x = fx.put(accel.acceleration.x) - gx.put(accel.acceleration.x);
    GV.accel.y = fy.put(accel.acceleration.y) - gy.put(accel.acceleration.y);
    GV.accel.z = fz.put(accel.acceleration.z) - gz.put(accel.acceleration.z);
}

AccelValue get()
{
    return AccelValue{fx.get(), fy.get(), fz.get()};
}

} // namespace Accel