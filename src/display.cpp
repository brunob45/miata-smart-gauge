#include "display.h"

#include "BtLogo.h"
#include "git_sha.h"
#include "global.h"
#include "miata.h"
#include "point.h"
#include "temperature.h"

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
uint16_t temp_565_cmap[256];
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
    // draw speedo background
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 12, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 11, GAUGE_BG);
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, GAUGE_BG);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, DISPLAY_ACCENT1);
    // tft.writeRect(0, 0, gimp_image.width, gimp_image.height, (uint16_t*)gimp_image.pixel_data);

    tft.setTextColor(0x7fe2);
    tft.setTextSize(2);

    // draw numbers around speedo
    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Point c = gaugeCenter - Point(a, gaugeRadius, Point::POLAR);
        tft.setCursor(c.x() - 4, c.y() - 7);
        tft.print(8 - i);
        a += 17;
    }
}

void updateTachoGauge()
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
    lastx = (GV.accel.y * radius / 9.81f) + center_x;
    lasty = (GV.accel.x * radius / 9.81f) + center_y;

    // draw accel marker
    tft.fillCircle(lastx, lasty, 4, DISPLAY_ACCENT1);
    // tft.drawLine(center_x, center_y, lastx, lasty, DISPLAY_FG2);
    init = true;
}

int16_t oilP = 0;
void updateBattGauge(GlobalVars* pGV)
{
    const int16_t oil_threshold = 50;

    // battery voltage compensation
    float adc_value = pGV->ms.sensors10;
    if (pGV->ms.batt > 0)
    {
        adc_value *= 1200.0f / pGV->ms.batt;
    }

    // const float m = -78.825f; // reading too high
    // const float b = 10565.0f;

    const float oilP_raw = -7.09425f * adc_value + 9008.5f;

    // threshold filter
    if (oilP_raw > oilP + oil_threshold)
    {
        oilP = oilP_raw - oil_threshold;
    }
    else if (oilP_raw + oil_threshold < oilP)
    {
        oilP = oilP_raw + oil_threshold;
    }

    tft.setTextSize(3);
    tft.setCursor(16, 180);
    if (adc_value == 0)
    {
        tft.print("...?");
    }
    else if (oilP < 0)
    {
        tft.print("LOW!");
    }
    else
    {
        printNum(oilP); // in g/cm2
    }

    tft.setCursor(16, 180 + 9 * 3);
    printNum(pGV->ms.batt);
}

void updateEgoGauge(GlobalVars* pGV)
{
    static uint8_t cursor = 0;
    static uint32_t last_update = 0;
    static uint8_t afrmin[80], afrmax[80];

    const int16_t x_offset = 28;
    const int16_t y_offset = 20;

    for (int i = 0; i < 80; i++)
    {
        uint8_t h1 = y_offset + 147 - max(min(afrmax[i], 167), 127);
        uint8_t h2 = y_offset + 147 - max(min(afrmin[i], 167), 127);
        tft.drawLine(x_offset + cursor, h1, x_offset + cursor, h2, ILI9341_GREEN);
    }
    tft.drawLine(x_offset + cursor, y_offset, x_offset + cursor, y_offset + 40, ILI9341_WHITE);
    tft.drawRect(x_offset, y_offset, 82, 42, ILI9341_WHITE);

    const bool doUpdate = millis() - last_update >= 100 && GV.connected;
    if (doUpdate)
    {
        last_update = millis();

        uint8_t cursor_next = (cursor < 79) ? cursor + 1 : 0;

        afrmax[cursor_next] = afrmin[cursor];
        afrmin[cursor_next] = afrmax[cursor];

        cursor = cursor_next;
    }

    afrmin[cursor] = min(afrmin[cursor], GV.ms.afr1);
    afrmax[cursor] = max(afrmax[cursor], GV.ms.afr1);
}

void updateIcons(GlobalVars* pGV)
{
    static bool overtemp;
    if (pGV->temperature > 85)
    {
        overtemp = true;
    }
    else if (pGV->temperature < 82)
    {
        overtemp = false;
    }
    if (overtemp || millis() < 7'000)
    {
        tft.writeRect8BPP(130, 16, temp_width, temp_height, temp_data, temp_565_cmap);
    }

    if (GV.ms.sensors9 > 300 || millis() < 7'000)
    {
        tft.writeRect8BPP(145, 16, bt_width, bt_height, bt_data, bt_565_cmap);
    }
}
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    get565cmap(miata_data_cmap, miata_565_cmap, 256);
    get565cmap(bt_data_cmap, bt_565_cmap, 256);
    get565cmap(temp_data_cmap, temp_565_cmap, 256);

    pinMode(A6, INPUT); // Night lights input
    pinMode(6, OUTPUT); // Brightness contol pin
    analogWrite(6, 0);  // Turn off brightness

    tft.begin(70e6);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.setRotation(3);

    tft.writeRect8BPP(0, 0, miata_width, miata_height, miata_data, miata_565_cmap);

    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setTextSize(1);
    tft.setCursor(45, 2);
    tft.print(GIT_SHA);

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

        updateTachoGauge();
        updateAccelGauge(66, 124, 32);
        updateEgoGauge(pGV);
        updateBattGauge(pGV);
        updateIcons(pGV);

        updateDisplay();
    }
}

int display_init(int prio, void* arg)
{
    chThdCreateStatic(waThdDisplay, sizeof(waThdDisplay), prio++, ThreadDisplay, arg);
    return 1;
}