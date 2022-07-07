#include "display.h"

#include "git_sha.h"
#include "global.h"
#include "miata.h"

#define TFT_CS 10
#define TFT_DC 9

namespace
{
THD_WORKING_AREA(waThdDisplay, 4 * 256);
THD_WORKING_AREA(waThdBacklight, 4 * 256);

DMAMEM uint16_t tft_frame_buffer0[240 * 320];
DMAMEM uint16_t tft_frame_buffer1[240 * 320];

ILI9341_t3n tft(TFT_CS, TFT_DC);

uint16_t header_565_cmap[256];
constexpr void get565cmap()
{
    for (uint16_t i = 0; i < 256; i++)
    {
        const uint8_t r = header_data_cmap[i][0];
        const uint8_t g = header_data_cmap[i][1];
        const uint8_t b = header_data_cmap[i][2];
        header_565_cmap[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
}

void updateDisplay()
{
    // Wait for previous refresh to finish
    // chEvtWaitAny(ILI9341_UPDATE_EVENT);
    while (tft.asyncUpdateActive())
    {
        chThdSleepMilliseconds(2);
    }
    // Start new refresh
    tft.updateScreenAsync();
    // Switch frame buffer
    if (tft.getFrameBuffer() == tft_frame_buffer0)
        tft.setFrameBuffer(tft_frame_buffer1);
    else
        tft.setFrameBuffer(tft_frame_buffer0);
}
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
    const float RATIO_BIN = 0.25f; // [0, 0.50]

    GlobalVars* pGV = (GlobalVars*)arg;

    get565cmap();

    tft.begin(70e6);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.setRotation(3);
    tft.attachThread(chThdGetSelfX());

    tft.writeRect8BPP(0, 0, width, height, header_data, header_565_cmap);
    updateDisplay();

    // Wait 2s for boot screen
    chThdSleepMilliseconds(2000);

    for (;;)
    {
        int x, y, x2, y2;
        for (x = 1; x < 15; x++)
        {
            if (pGV->ms.rpm <= pGV->ms.rpm_table[x])
                break;
        }
        {
            float rpm1 = pGV->ms.rpm_table[x - 1];
            float rpm2 = pGV->ms.rpm_table[x];
            float ax = (pGV->ms.rpm - rpm1) / (rpm2 - rpm1);
            if (ax <= RATIO_BIN)
            {
                // value near rpm1
                x = x - 1;
                x2 = x;
            }
            else if (ax >= (1 - RATIO_BIN))
            {
                // value near rpm2
                x2 = x;
            }
            else
            {
                // value in the middle
                x2 = x;
                x = x - 1;
            }
        }

        for (y = 1; y < 15; y++)
        {
            if (pGV->ms.map <= pGV->ms.map_table[y])
                break;
        }
        {
            float map1 = pGV->ms.map_table[y - 1];
            float map2 = pGV->ms.map_table[y];
            float ay = (pGV->ms.map - map1) / (map2 - map1);
            if (ay <= RATIO_BIN)
            {
                y = y - 1;
                y2 = y;
            }
            else if (ay >= (1 - RATIO_BIN))
            {
                y2 = y;
            }
            else
            {
                y2 = y;
                y = y - 1;
            }
        }

        tft.fillScreen(ILI9341_BLACK);

        tft.setTextSize(5);
        tft.setCursor(40, 20);
        tft.setTextColor(ILI9341_GREENYELLOW, ILI9341_BLACK);

        uint16_t val = pGV->ms.rpm;
        if (val < 10) tft.print(' ');
        if (val < 100) tft.print(' ');
        if (val < 1000) tft.print(' ');
        tft.print(val);

        tft.print('|');

        val = pGV->ms.map;
        if (val < 10) tft.print(' ');
        if (val < 100) tft.print(' ');
        if (val < 1000) tft.print(' ');
        tft.print(val);

        tft.setTextSize(1);
        for (int j = 0; j < 16; j++)
        {
            for (int i = 0; i < 16; i++)
            {
                const uint8_t val = pGV->ms.vetable[i + j * 16];
                if ((i == x || i == x2) && (j == y || j == y2))
                    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                else if (val < 98)
                    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
                else if (val > 102)
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
                else
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);

                tft.setCursor(20 * i, -10 * j + 230);
                if (val < 10) tft.print(' ');
                if (val < 100) tft.print(' ');
                tft.print(val);
            }
        }
        updateDisplay();
    }
}

THD_FUNCTION(ThreadBacklight, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    pinMode(6, OUTPUT); // Brightness contol pin
    pinMode(A6, INPUT); // Night lights input

    analogWrite(6, 0); // Turn off brightness

    for (;;)
    {
        pGV->lumi = analogRead(A6);
        analogWrite(6, (pGV->lumi > 512) ? 30 : 255);
        chThdSleepMilliseconds(100);
    }
}

int display_init(int prio, void* arg)
{
    chThdCreateStatic(waThdDisplay, sizeof(waThdDisplay), prio++, ThreadDisplay, arg);
    chThdCreateStatic(waThdBacklight, sizeof(waThdBacklight), prio++, ThreadBacklight, arg);
    return 2;
}