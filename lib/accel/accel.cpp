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

    // invert x & z axis to orient the board correctly
    const float ax = accel.acceleration.z * 0.969367589f + 0.465296443f;
    const float ay = accel.acceleration.y * 0.993920973f + 0.079513678f;
    const float az = accel.acceleration.x * 0.991911021f - 0.386845298f;
    Quaternion qa(ax, ay, -az);

    // correct gyro offset to minimize drift
    const float gx = gyro.gyro.z + 0.01610f;
    const float gy = gyro.gyro.y + 0.01491f;
    const float gz = gyro.gyro.x + 0.01203f;
    Quaternion qg(gx, gy, -gz);

    deltat = fusion.deltatUpdate(); // this have to be done before calling the fusion update
    fusion.MahonyUpdate(            // mahony is suggested if there isn't the mag and the mcu is slow
        qg.x, qg.y, qg.z,
        qa.x, qa.y, qa.z,
        deltat);

    Quaternion q2 = Quaternion::from_euler_rotation(
        fusion.getRollRadians(),
        fusion.getPitchRadians(),
        0); // fusion.getYawRadians()); // ignore yaw orientation

    // moving window filter
    avgsum += (avgbuf[avgindex] * -1);
    avgbuf[avgindex] = q2.rotate(qa); // rotate acceleration by orientation to get the Z axis pointing up
    avgsum += avgbuf[avgindex];
    avgindex = (avgindex + 1) % avgsize;
    Quaternion avg = avgsum * (1.0 / avgsize);

    GV.accel.x = avg.x;
    GV.accel.y = avg.y;
    GV.accel.z = avg.z - 9.81f; // remove gravity
}

} // namespace Accel