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

const int avgsize = 50;
float avgx[avgsize], avgy[avgsize], avgz[avgsize];
float x, y, z;
int avgindex;

// filter constant = "refresh rate" / "signal lag" (in seconds)
FilterClass fx(0.020f / 0.60f), fy(0.020f / 0.60f), fz(0.020f / 0.60f);
FilterClass gx(0.020f / 10.0f), gy(0.020f / 10.0f), gz(0.020f / 10.0f);
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

    Quaternion q(accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);

    deltat = fusion.deltatUpdate();                                    // this have to be done before calling the fusion update
    fusion.MahonyUpdate(                                               // mahony is suggested if there isn't the mag and the mcu is slow
        gyro.gyro.x + 0.01f, gyro.gyro.y + 0.01f, gyro.gyro.z + 0.02f, // correct gyro offset to minimize drift
        accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
        deltat);

    Quaternion q2;
    memcpy((void*)&q2, fusion.getQuat(), sizeof(float) * 4);
    Quaternion q3 = q2.rotate(q); // rotate acceleration by orientation to get the Z axis pointing up

    x -= avgx[avgindex];
    avgx[avgindex] = q3.b;
    x += avgx[avgindex];
    y -= avgy[avgindex];
    avgy[avgindex] = q3.c;
    y += avgy[avgindex];
    z -= avgz[avgindex];
    avgz[avgindex] = q3.d;
    z += avgz[avgindex];
    avgindex = (avgindex + 1) % avgsize;

    GV.accel.x = x / avgsize + 0.07f;
    GV.accel.y = y / avgsize - 0.07f;
    GV.accel.z = z / avgsize - 9.57f;

    // GV.accel.x = fusion.getYaw();
    // GV.accel.y = fusion.getPitch(); // you could also use getRollRadians() ecc
    // GV.accel.z = fusion.getRoll();

    // Update gravity (low pass filter)
    // Update accel (band pass filter)
    // GV.accel.x = fx.put(accel.acceleration.x) - gx.put(accel.acceleration.x);
    // GV.accel.y = fy.put(accel.acceleration.y) - gy.put(accel.acceleration.y);
    // GV.accel.z = fz.put(accel.acceleration.z) - gz.put(accel.acceleration.z);
}

AccelValue get()
{
    return AccelValue{fx.get(), fy.get(), fz.get()};
}

} // namespace Accel