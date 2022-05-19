
#include <Arduino.h>
#include <ChRt.h>

#include "accel.h"
#include "canbus.h"
#include "current_status.h"
#include "display.h"
#include "global.h"
#include "speedo.h"

GlobalVars GV;

void checkFault(uint16_t* code, uint8_t index, bool set, bool reset)
{
    if (code)
    {
        if (set)
        {
            // inverted logic : 0 = fault active, 1 = fault inactive
            *code &= ~(1 << index);
        }
        else if (reset)
        {
            *code |= (1 << index);
        }
    }
}

THD_WORKING_AREA(waThdMain, 4 * 256);
THD_FUNCTION(ThreadMain, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    pinMode(A6, INPUT);

    Serial.begin(115200);

    Accel::init();
    Speedo::init();
    CanBus::init();
    Display::init();

    uint16_t last_fault = 0;
    uint32_t last_fault_change = 0;

    for (;;)
    {
        Accel::update();
        Speedo::update();
        CanBus::update();

        pGV->alert = pGV->ms.rpm > 7200;

        pGV->lumi = analogRead(A6);
        if (Display::isReady())
        {
            analogWrite(6, (pGV->lumi > 512) ? 30 : 255);
        }

        if (millis() - last_fault_change > 500)
        {
            // High coolant temperature
            checkFault(&pGV->fault_code, 0, pGV->ms.clt > 2120, pGV->ms.clt <= 2000); // 100C & 93C

            // Low oil pressure
            checkFault(&pGV->fault_code, 1, pGV->ms.sensors2 > 150, pGV->ms.sensors2 < 140);

            // Engine off
            checkFault(&pGV->fault_code, 2, pGV->ms.rpm < 50, pGV->ms.rpm > 200);

            if (last_fault != pGV->fault_code)
            {
                last_fault_change = millis();
                last_fault = pGV->fault_code;
            }
            Serial.println(chUnusedThreadStack(waThdMain, sizeof(waThdMain)));
        }

        Display::update();
    }
}

void chSetup()
{
    chThdCreateStatic(waThdMain, sizeof(waThdMain), NORMALPRIO, ThreadMain, &GV);
}

void setup()
{
    chBegin(chSetup);
}

void loop(void)
{
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
}
