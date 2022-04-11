#include "gps.h"

#include <Adafruit_GPS.h>
#include <Arduino.h>
#include <ChRt.h>

#include "global.h"

#define SerialGPS Serial3

namespace GPS
{
namespace
{
THD_WORKING_AREA(waThdGPS, 14 * 256);

THD_FUNCTION(ThreadGPS, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    Adafruit_GPS myGPS(&SerialGPS);

    pGV->gps.fix = false;

    myGPS.begin(9600);

    chThdSleepMilliseconds(1500);
    myGPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    myGPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
    myGPS.sendCommand(PMTK_SET_BAUD_57600);
    SerialGPS.end();
    myGPS.begin(57600);

    for (;;)
    {
        myGPS.read();

        if (myGPS.newNMEAreceived())
        {
            myGPS.parse(myGPS.lastNMEA());

            pGV->gps.fix = myGPS.fix;
            pGV->gps.dir = myGPS.angle;
            pGV->gps.spd = myGPS.speed * 1.852f; // knots to km/h
            pGV->gps.tod = ((myGPS.hour + 20) % 24) * 1800 + myGPS.minute * 30 + myGPS.seconds / 2;
        }
        pGV->gps.stack = chUnusedThreadStack(waThdGPS, sizeof(waThdGPS));
        chThdSleepMilliseconds(20); // 5Hz refresh rate
    }
}
} // namespace

void initThreads(tprio_t prio, void* arg)
{
    chThdCreateStatic(waThdGPS, sizeof(waThdGPS), prio, ThreadGPS, arg);
}
} // namespace GPS