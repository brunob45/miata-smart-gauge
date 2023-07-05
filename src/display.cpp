#include "display.h"

#include "BtLogo.h"
#include "git_sha.h"
#include "global.h"
#include "miata.h"
#include "point.h"

#define TFT_CS 10
#define TFT_DC 9

#define DISPLAY_ALERT 0xF800 // ILI9341_RED
#define DISPLAY_ACCENT1 0x7fe2
#define DISPLAY_ACCENT2 0x39e7
#define DISPLAY_FG1 0xC618 // ILI9341_LIGHTGREY
#define DISPLAY_FG2 0x7BEF // ILI9341_DARKGREY
#define DISPLAY_BG ILI9341_BLACK

namespace
{
THD_WORKING_AREA(waThdDisplay, 6 * 256);

DMAMEM uint16_t tft_frame_buffer0[240 * 320];
DMAMEM uint16_t tft_frame_buffer1[240 * 320];

Point gaugeCenter(320 - 52, 240 - 82, Point::CARTESIAN);
int16_t gaugeRadius = 140;
uint16_t GAUGE_BG = DISPLAY_BG;

ILI9341_t3n tft(TFT_CS, TFT_DC);

constexpr uint16_t rgb_to_565(int r, int g, int b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t miata_565_cmap[256];
uint16_t bt_565_cmap[256];
constexpr void get565cmap(uint8_t cmap_rgb[][3], uint16_t* cmap_565, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        const uint8_t r = cmap_rgb[i][0];
        const uint8_t g = cmap_rgb[i][1];
        const uint8_t b = cmap_rgb[i][2];
        cmap_565[i] = rgb_to_565(r, g, b);
    }
}

uint8_t numSize(int n)
{
    uint8_t s = (n < 0);
    n = abs(n);
    if (n < 100) // small binary-search-like optimisation
    {
        if (n < 10) // n between 0 & 9
            s += 1;
        else // n between 10 & 99
            s += 2;
    }
    else if (n < 10000) // n between 100 & 9999
    {
        if (n < 1000) // n between 100 & 999
            s += 3;
        else // n between 1000 & 9999
            s += 4;
    }
    else // n between 10000 & 65535, maximum value for 16-bit integer
        s += 5;
    return s;
}

void updateDisplay()
{
    // Wait for previous refresh to finish
    tft.waitUpdateAsyncComplete();

    // Start new refresh
    tft.updateScreenAsync();

    // Switch frame buffer
    if (tft.getFrameBuffer() == tft_frame_buffer0)
        tft.setFrameBuffer(tft_frame_buffer1);
    else
        tft.setFrameBuffer(tft_frame_buffer0);
}

uint16_t updateBrightness()
{
    uint16_t lumi = analogRead(A6);
    analogWrite(6, (lumi > 512) ? 30 : 255);
    return lumi;
}

void printNum(int16_t num)
{
    if (num < 0)
    {
        tft.print('-');
        num = abs(num);
    }
    else
    {
        if (num < 1000) tft.print(' ');
    }

    if (num < 100) tft.print(' ');
    if (num < 10) tft.print(' ');

    tft.print(num);
}

void initGauge()
{
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 12, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 11, GAUGE_BG);
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, GAUGE_BG);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, DISPLAY_ACCENT1);
    // tft.writeRect(0, 0, gimp_image.width, gimp_image.height, (uint16_t*)gimp_image.pixel_data);

    tft.setTextColor(0x7fe2);
    tft.setTextSize(2);

    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Point c = gaugeCenter - Point(a, gaugeRadius, Point::POLAR);
        tft.setCursor(c.x() - 4, c.y() - 7);
        tft.print(8 - i);
        a += 17;
    }
}

void updateGauge()
{
    static Point needle[2];
    static bool wasAlert = false;

    initGauge();

    if (GV.alert != wasAlert)
    {
        GAUGE_BG = GV.alert ? DISPLAY_ALERT : DISPLAY_BG;
        tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 14, GAUGE_BG);
        wasAlert = GV.alert;
    }

    Point c1(-(GV.ms.rpm * 17 / 1000) + 119, gaugeRadius - 16, Point::POLAR);
    Point c2(-(GV.ms.rpm * 17 / 1000) + 119, -30, Point::POLAR);

    // tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), GAUGE_BG);
    // tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), GAUGE_BG);
    // tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, GAUGE_BG);

    needle[0] = gaugeCenter - c2;
    needle[1] = gaugeCenter - c1;

    tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, DISPLAY_FG1);

    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 9, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 8, DISPLAY_ACCENT1);

    tft.setTextColor(DISPLAY_FG1, GAUGE_BG);
    tft.setTextSize(4);
    tft.setCursor(220, 205);

    for (int i = 0; i < 4 - numSize(GV.ms.rpm); i++)
    {
        tft.print(' ');
    }
    tft.print(GV.ms.rpm);
}

void updateAccelGauge(uint16_t center_x, uint16_t center_y, uint16_t radius)
{
    static int16_t lastx, lasty;
    static bool init = false;

    if (init)
    {
        // remove last point
        tft.fillCircle(lastx, lasty, 4, DISPLAY_BG);
    }

    // re-draw gauge background
    tft.drawCircle(center_x, center_y, radius, DISPLAY_FG1);
    tft.drawCircle(center_x, center_y, radius / 2, DISPLAY_FG1);
    tft.drawFastHLine(center_x - radius, center_y, radius * 2, DISPLAY_FG1);
    tft.drawFastVLine(center_x, center_y - radius, radius * 2, DISPLAY_FG1);

    // update accel marker position
    lastx = (-GV.accel.x * radius) + center_x;
    lasty = (GV.accel.z * radius) + center_y;

    // draw accel marker
    tft.fillCircle(lastx, lasty, 4, DISPLAY_ACCENT1);
    // tft.drawLine(center_x, center_y, lastx, lasty, DISPLAY_FG2);
    init = true;
}
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    get565cmap(miata_data_cmap, miata_565_cmap, 256);
    get565cmap(bt_data_cmap, bt_565_cmap, 256);

    pinMode(A6, INPUT); // Night lights input
    pinMode(6, OUTPUT); // Brightness contol pin
    analogWrite(6, 0);  // Turn off brightness

    tft.begin(70e6);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.setRotation(3);

    tft.writeRect8BPP(0, 0, miata_width, miata_height, miata_data, miata_565_cmap);
    tft.updateScreenAsync();
    tft.waitUpdateAsyncComplete();

    // Wait 2s for boot screen
    for (int i = 0; i < 20; i++)
    {
        pGV->lumi = updateBrightness();
        chThdSleepMilliseconds(100);
    }

    for (;;)
    {
        pGV->lumi = updateBrightness();

        tft.fillScreen(ILI9341_BLACK);

        updateGauge();
        updateAccelGauge(66, 124, 32);

        float ego_scale = 1.0f;
        if (pGV->ltt.error < 0.5f)
        {
            ego_scale = -1.0f;
        }
        else if (pGV->ltt.error < 1.0f)
        {
            ego_scale = 1.0f - 1.0f / pGV->ltt.error;
        }
        else if (pGV->ltt.error < 2.0f)
        {
            ego_scale = pGV->ltt.error - 1.0f;
        }
        // else ego_scale = 1.0f

        const int16_t scale_width = 96;
        const int16_t offset = ego_scale * scale_width / 2;
        if (offset < 0)
        {
            tft.fillRect(28 + scale_width / 2 + offset, 12, -offset, 34, ILI9341_GREENYELLOW);
        }
        else
        {
            tft.fillRect(28 + scale_width / 2, 12, offset, 34, ILI9341_GREENYELLOW);
        }
        tft.drawRect(28, 12, scale_width, 34, ILI9341_WHITE);

        tft.setTextColor(ILI9341_GREENYELLOW, ILI9341_BLACK);
        tft.setTextSize(1);
        tft.setCursor(28 - 9, 48);
        tft.print("1/2");

        tft.setCursor(28 + 96 / 2 - 9, 48);
        tft.print("1/1");

        tft.setCursor(28 + 96 - 9, 48);
        tft.print("2/1");

        const float oil_a = -0.008352f;
        const float oil_b = -0.5f;
        const float oil_c = 116.0f;

        tft.setTextSize(3);
        tft.setCursor(16, 180);
        if (pGV->ms.sensors10 == 0)
        {
            tft.print("....");
        }
        else if (pGV->ms.sensors10 > 91)
        {
            tft.print("LOW!");
        }
        else
        {
            const float oil_pressure = oil_a * pGV->ms.sensors10 * pGV->ms.sensors10 + oil_b * pGV->ms.sensors10 + oil_c;
            printNum(oil_pressure * 70.307f); // convert psi to g/cm2
        }

        tft.setCursor(16, 180 + 9 * 3);
        printNum(pGV->ltt.error * 1000);

        // if (pGV->ltt.engaged)
        // {
        //     tft.fillCircle(6, 240 - 50, 4, ILI9341_GREEN);
        // }
        // else
        // {
        //     tft.drawCircle(6, 240 - 50, 4, ILI9341_GREEN);
        // }
        // if (pGV->ltt.needBurn)
        // {
        //     tft.fillCircle(6, 240 - 35, 4, ILI9341_YELLOW);
        // }
        // else
        // {
        //     tft.drawCircle(6, 240 - 35, 4, ILI9341_YELLOW);
        // }
        static bool overtemp;
        if (pGV->temperature > 85)
        {
            overtemp = true;
        }
        else if (pGV->temperature < 82)
        {
            overtemp = false;
        }
        if (overtemp)
        {
            tft.fillCircle(134, 16, 4, ILI9341_RED);
        }
        else
        {
            tft.drawCircle(134, 16, 4, ILI9341_RED);
        }

        // tft.setTextSize(1);
        // for (int j = 0; j < 16; j++)
        // {
        //     for (int i = 0; i < 16; i++)
        //     {
        //         const uint8_t index = i + j * 16;
        //         uint16_t color = rgb_to_565(50, 50, 50);
        //         if ((i == pGV->ltt.x[0] || i == pGV->ltt.x[1]) &&
        //             (j == pGV->ltt.y[0] || j == pGV->ltt.y[1]))
        //         {
        //             color = ILI9341_YELLOW;
        //         }
        //         else if (pGV->ltt.err[index] == EGOERR::RICH)
        //         {
        //             color = ILI9341_CYAN;
        //         }
        //         else if (pGV->ltt.err[index] == EGOERR::LEAN)
        //         {
        //             color = ILI9341_ORANGE;
        //         }
        //         else if (pGV->ltt.err[index] == EGOERR::OK)
        //         {
        //             color = ILI9341_GREEN;
        //         }

        //         const int sizex = 6, sizey = 4;
        //         tft.fillRect(sizex * i + 16, -sizey * j + (240 - sizey), sizex, sizey, color);
        //     }
        // }

        tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
        tft.setTextSize(1);
        tft.setCursor(45, 2);
        tft.print(GIT_SHA);

        if (GV.ms.sensors9 > 300)
        {
            tft.writeRect8BPP(2, 150, bt_width, bt_height, bt_data, bt_565_cmap);
        }

        updateDisplay();
    }
}

int display_init(int prio, void* arg)
{
    chThdCreateStatic(waThdDisplay, sizeof(waThdDisplay), prio++, ThreadDisplay, arg);
    return 1;
}