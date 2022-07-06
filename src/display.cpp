#include "display.h"

#include "git_sha.h"
#include "global.h"
#include "miata.h"

#define TFT_CS 10
#define TFT_DC 9

namespace
{
THD_WORKING_AREA(waThdDisplay, 4 * 256);

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

uint16_t updateBrightness()
{
    uint16_t bright = analogRead(A6);
    analogWrite(6, (bright > 512) ? 30 : 255);
    return bright;
}

void updateDisplay()
{
    tft.updateScreenAsync();
    while (tft.asyncUpdateActive())
    {
        chThdSleepMilliseconds(2);
    }
}
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
    const float RATIO_BIN = 0.3f; // [0, 0.5]
    GlobalVars* pGV = (GlobalVars*)arg;

    pinMode(6, OUTPUT);
    digitalWrite(6, LOW);
    pinMode(A6, INPUT);

    get565cmap();

    tft.begin(70e6);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.setRotation(3);

    tft.writeRect8BPP(0, 0, width, height, header_data, header_565_cmap);
    updateDisplay();

    pGV->lumi = updateBrightness();

    // Wait 2s for boot screen
    chThdSleepMilliseconds(2000);

    for (;;)
    {
        pGV->lumi = updateBrightness();

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

        int x, y, x2, y2;
        for (x = 0; x < 16; x++)
        {
            if (pGV->ms.rpm <= pGV->ms.rpm_table[x + 1]) break;
        }
        if (x > 0 && x < 16)
        {
            float rpm1 = pGV->ms.rpm_table[x - 1];
            float rpm2 = pGV->ms.rpm_table[x];
            float alpha = (pGV->ms.rpm - rpm1) / (rpm2 - rpm1);
            if (alpha <= RATIO_BIN)
            {
                // value near rpm1
                x = x - 1;
                x2 = x;
            }
            else if (alpha >= (1 - RATIO_BIN))
            {
                // value near rpm2
                x2 = x;
            }
            else
            {
                // value in the middle
                x2 = x - 1;
            }
        }
        else
        {
            x2 = x;
        }
        for (y = 0; y < 16; y++)
        {
            if (pGV->ms.map <= pGV->ms.map_table[y + 1]) break;
        }
        if (x > 0 && x < 16)
        {
            float map1 = pGV->ms.map_table[y - 1];
            float map2 = pGV->ms.map_table[y];
            float alpha = (pGV->ms.map - map1) / (map2 - map1);
            if (alpha <= RATIO_BIN)
            {
                y = y - 1;
                y2 = y;
            }
            else if (alpha >= (1 - RATIO_BIN))
            {
                y2 = y;
            }
            else
            {
                y2 = y - 1;
            }
        }
        else
        {
            y2 = y;
        }
        
        tft.setTextSize(1);
        for (int j = 15; j >= 0; j--)
        {
            for (int i = 0; i < 16; i++)
            {
                tft.setCursor(20 * i, 10 * j + 80);
                const uint8_t val = pGV->ms.vetable[i + j * 16];
                if ((i == x || i == x2) && (j == y || j == y2))
                    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                else if (val < 98)
                    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
                else if (val > 102)
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
                else
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);

                if (val < 10) tft.print(' ');
                if (val < 100) tft.print(' ');
                tft.print(val);
            }
        }
        updateDisplay();
    }
}

void display_init(int prio, void* arg)
{
    chThdCreateStatic(waThdDisplay, sizeof(waThdDisplay), prio, ThreadDisplay, arg);
}