#include "accel.h"

#include <Adafruit_MPU6050.h>
#include <SensorFusion.h>

#include "global.h"

namespace Accel
{
namespace
{
Adafruit_MPU6050 mpu;
sensors_event_t accel, gyro, temp;
SF fusion;

uint32_t last_tx;
} // namespace

void init(void)
{
    // Init MPU6050
    mpu.begin();
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    last_tx = millis();
}

void update(void)
{
    static float sum[3];
    static uint32_t count;

    // Update accel values
    mpu.getEvent(&accel, &gyro, &temp);

    // invert x & z axis to orient the board correctly
    const float ax = accel.acceleration.z * 0.969367589f + 0.465296443f;  // [-10.60,9.64]
    const float ay = accel.acceleration.y * 0.993920973f + 0.079513678f;  // [-9.95,9.79]
    const float az = -accel.acceleration.x * 0.991911021f + 0.386845298f; // [-9.50,10.28]

    // correct gyro offset to minimize drift
    const float gx = gyro.gyro.z + 0.026756891f;
    const float gy = gyro.gyro.y + 0.023081699f;
    const float gz = gyro.gyro.x + 0.000516514f;

    sum[0] += gx;
    sum[1] += gy;
    sum[2] += gz;
    count += 1;

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
        if (false)
        {
            // gyro calibration
            Serial.print(sum[0]);
            Serial.print(',');
            Serial.print(sum[1]);
            Serial.print(',');
            Serial.print(sum[2]);
            Serial.print(',');
            Serial.print(count);
        }
        else
        {
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
        }
        Serial.println();
    }
}

} // namespace Accel