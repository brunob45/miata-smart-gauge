#include "display.h"

#include <Arduino.h>

#include "global.h"
#include "point.h"

namespace Display
{
namespace Internal
{
Point gaugeCenter(320 - 52, 240 - 82, Point::CARTESIAN);
int16_t gaugeRadius = 140;
uint16_t GAUGE_BG = DISPLAY_BG;

extern ILI9341_t3 tft;

uint8_t numSize(uint16_t n)
{
    uint8_t s = 0;
    do
    {
        n /= 10;
        s++;
    } while (n > 0);

    return s;
}

void initGauge()
{
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 12, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 11, GAUGE_BG);
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, GAUGE_BG);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, DISPLAY_ACCENT1);

    tft.setTextSize(2);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setTextColor(0x7fe2);

    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Point c = gaugeCenter - Point(a, gaugeRadius, Point::POLAR);
        tft.setCursor(c.x() - 4, c.y() - 7);
        tft.print(8 - i);
        a += 17;
    }
}

void initMenu1()
{
    tft.fillScreen(DISPLAY_BG);

    initGauge();

    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setCursor(5, 75);
    tft.print("x");
    tft.setCursor(5, 75 + 50);
    tft.print("y");
    tft.setCursor(5, 75 + 100);
    tft.print("map");
}

void initMenu2()
{
    tft.fillScreen(DISPLAY_BG);
    initGauge();
}

void updateGauge()
{
    static Point needle[2];
    static bool wasAlert = false;
    static bool wasConnected = false;

    if (GV.connected)
    {
        if (GV.alert != wasAlert)
        {
            GAUGE_BG = GV.alert ? DISPLAY_ALERT : DISPLAY_BG;
            tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 14, GAUGE_BG);
            wasAlert = GV.alert;
        }

        Point c1(-(GV.ms.rpm * 17 / 1000) + 119, gaugeRadius - 16, Point::POLAR);
        Point c2(-(GV.ms.rpm * 17 / 1000) + 119, -30, Point::POLAR);

        tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), GAUGE_BG);
        tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), GAUGE_BG);
        tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, GAUGE_BG);

        needle[0] = gaugeCenter - c2;
        needle[1] = gaugeCenter - c1;

        tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), DISPLAY_FG1);
        tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), DISPLAY_FG1);
        tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, DISPLAY_FG1);

        tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 9, DISPLAY_ACCENT2);
        tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 8, DISPLAY_ACCENT1);

        tft.setTextSize(4);
        tft.setTextColor(DISPLAY_FG1, GAUGE_BG);
        tft.setCursor(220, 205);

        for (int i = 0; i < 4 - numSize(GV.ms.rpm); i++)
        {
            tft.print(' ');
        }
        tft.print(GV.ms.rpm);
    }
    else
    {
        if (wasConnected)
        {
            GAUGE_BG = DISPLAY_BG;
            tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 14, GAUGE_BG);
            wasAlert = false;
        }
    }
    wasConnected = GV.connected;
}

void drawNumber(int number, int scale, int offset, int x, int y)
{
    const int fontsize = 3;
    const int charsize = fontsize * 6; // a char is 6 pixel wide

    tft.setTextSize(fontsize);
    tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
    tft.setCursor(x, y);

    const int whole = number / scale;
    const int decimal = number % scale;

    // Print left padding with "space" character
    for(int i = numSize(whole); i < offset; i++)
    {
        tft.print(' ');
    }

    // print whole part of number
    tft.print(whole);

    if (scale > 1)
    {
        // print decimal part of number
        const int scale_size = numSize(scale) - 1;
        const int cursor_x = (offset * charsize) + x;

        tft.setCursor(cursor_x + (fontsize * 3), y);
        for (int i = numSize(decimal); i < scale_size; i++)
        {
            tft.print('0');
        }
        tft.print(decimal);

        // print dot
        tft.setTextColor(DISPLAY_FG1, DISPLAY_FG1);
        tft.setCursor(cursor_x - (fontsize * 2), y);
        tft.print('.');
    }
}

void updateMenu1()
{
    updateGauge();

    drawNumber(abs(GV.accel.z) * 100, 100, 1, 5, 92);
    drawNumber(abs(GV.accel.x) * 100, 100, 1, 5, 92 + 50);
    drawNumber(GV.ms.map, 10, 3, 5, 92 + 100);
}

void updateMenu2()
{
    updateGauge();

    tft.setTextSize(10);
    tft.setCursor(30, 100);

    if (GV.gear > 0)
    {
        tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
        tft.print(GV.gear);
    }
    else
    {
        tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
        tft.print('?');
    }
}

} // namespace Internal
} // namespace Display