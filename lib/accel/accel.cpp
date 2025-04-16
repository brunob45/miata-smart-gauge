#include "accel.h"

#include <Adafruit_MPU6050.h>
// #include <Quaternion.h>
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

uint32_t last_tx;

// Quaternion filter;
// const int filtersize = 50;
} // namespace

void init(void)
{
    // Init MPU6050
    mpu.begin();
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    last_tx = millis();
}

void update(void)
{
    // Update accel values
    mpu.getEvent(&accel, &gyro, &temp);

    // invert x & z axis to orient the board correctly
    const float ax = accel.acceleration.z * 0.969367589f + 0.465296443f;  // [-10.60,9.64]
    const float ay = accel.acceleration.y * 0.993920973f + 0.079513678f;  // [-9.95,9.79]
    const float az = -accel.acceleration.x * 0.991911021f + 0.386845298f; // [-9.50,10.28]

    // correct gyro offset to minimize drift
    const float gx = gyro.gyro.z + 0.01610f;
    const float gy = gyro.gyro.y + 0.01491f;
    const float gz = gyro.gyro.x + 0.01203f;

    const float deltat = fusion.deltatUpdate(); // this have to be done before calling the fusion update
    fusion.MahonyUpdate(                        // mahony is suggested if there isn't the mag and the mcu is slow
        gx, gy, gz,
        ax, ay, az,
        deltat);

    GV.accel.x = ax;
    GV.accel.y = ay;
    GV.accel.z = az - 9.81f;

    if (millis() - last_tx > 100)
    {
        last_tx = millis();
        Serial.print(ax);
        Serial.print(',');
        Serial.print(ay);
        Serial.print(',');
        Serial.print(az);
        Serial.print(',');
        Serial.print(gx);
        Serial.print(',');
        Serial.print(gy);
        Serial.print(',');
        Serial.print(gz);
        Serial.print(',');
        Serial.print(fusion.getYaw());
        Serial.print(',');
        Serial.print(fusion.getPitch());
        Serial.print(',');
        Serial.print(fusion.getRoll());
        Serial.print(',');
        Serial.print(temp.temperature);
        Serial.println();
    }
    // Quaternion qf;
    // memcpy((void*)&qf, (void*)fusion.getQuat(), sizeof(float) * 4);
    // qf = qf.to_euler();

    // qf = Quaternion::from_euler_rotation(
    //     qf.roll,
    //     qf.pitch,
    //     0); // qfe.yaw); // ignore yaw orientation

    // qa = qf.rotate(qa); // rotate acceleration by orientation to get the Z axis pointing up

    // filter = (filter * (filtersize - 1) + qa) * (1.0f / filtersize);

    // GV.accel.x = filter.x;
    // GV.accel.y = filter.y;
    // GV.accel.z = filter.z - 9.81f; // remove gravity
}

} // namespace Accel