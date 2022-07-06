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
    while(tft.asyncUpdateActive())
    {
        chThdSleepMilliseconds(2);
    }
}
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
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
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(0,0);

        tft.setTextSize(5);
        tft.setCursor(160, 20);

        uint16_t val = pGV->ms.rpm;
        if (val < 10) tft.print(' ');
        if (val < 100) tft.print(' ');
        if (val < 1000) tft.print(' ');
        tft.print(val);

        tft.setTextSize(1);
        int x, y;
        for (x = 0; x < 15; x++)
        {
            if (pGV->ms.rpm <= pGV->ms.rpm_table[x+1]) break;
        }
        for (y = 0; y < 15; y++)
        {
            if (pGV->ms.map <= pGV->ms.map_table[y+1]) break;
        }
        for(int j = 0; j < 16; j++)
        {
            for(int i = 0; i < 16; i++)
            {
                tft.setCursor(20*i, 10*j+80);
                const uint8_t val = pGV->ms.vetable[i+(15-j)*16];
                if (i == x && j == y)
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