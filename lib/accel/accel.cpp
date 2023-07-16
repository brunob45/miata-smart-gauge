#include "accel.h"

#include <Adafruit_MPU6050.h>
#include <Quaternion.h>
#include <SensorFusion.h>

#include "filter.h"
#include "global.h"

namespace Accel
{
namespace
{
Adafruit_MPU6050 mpu;
sensors_event_t accel, gyro, temp;
SF fusion;
float deltat;

// moving window filter
const int avgsize = 50;
Quaternion avgbuf[avgsize];
Quaternion avgsum;
int avgindex;
} // namespace

void init(void)
{
    // Init MSA301
    mpu.begin();
}

void update(void)
{
    // Update accel values
    mpu.getEvent(&accel, &gyro, &temp);

    Quaternion q(accel.acceleration.z, accel.acceleration.y, -accel.acceleration.x);

    deltat = fusion.deltatUpdate();                                        // this have to be done before calling the fusion update
    fusion.MahonyUpdate(                                                   // mahony is suggested if there isn't the mag and the mcu is slow
        gyro.gyro.z + 0.02f, gyro.gyro.y + 0.01f, -gyro.gyro.x - 0.01f,    // correct gyro offset to minimize drift
        accel.acceleration.z, accel.acceleration.y, -accel.acceleration.x, // invert x & z axis to orient the board correctly
        deltat);

    Quaternion q2;
    q2 = Quaternion::from_euler_rotation(
        fusion.getRollRadians(),
        fusion.getPitchRadians(),
        0); // fusion.getYawRadians()); // ignore yaw orientation

    // moving window filter
    avgsum += (avgbuf[avgindex] * -1);
    avgbuf[avgindex] = q2.rotate(q); // rotate acceleration by orientation to get the Z axis pointing up
    avgsum += avgbuf[avgindex];
    avgindex = (avgindex + 1) % avgsize;
    Quaternion avg = avgsum * (1.0 / avgsize);

    GV.accel.x = avg.b + 0.07f;
    GV.accel.y = avg.c - 0.07f;
    GV.accel.z = avg.d - 9.57f;
}

} // namespace Accel