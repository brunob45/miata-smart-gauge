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
} // namespace

THD_FUNCTION(ThreadDisplay, arg)
{
    GlobalVars* pGV = (GlobalVars*)arg;

    get565cmap();

    pinMode(A6, INPUT); // Night lights input
    pinMode(6, OUTPUT); // Brightness contol pin
    analogWrite(6, 0);  // Turn off brightness

    tft.begin(70e6);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.setRotation(3);

    tft.writeRect8BPP(0, 0, width, height, header_data, header_565_cmap);
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

        tft.setTextSize(4);
        tft.setTextColor(ILI9341_GREENYELLOW, ILI9341_BLACK);

        tft.setCursor(40, 6);
        printNum(pGV->ms.rpm);
        tft.print('|');
        printNum(pGV->ms.map);

        tft.setCursor(40, 40);
        printNum(pGV->ltt.error * 1000);
        tft.print('|');
        printNum(pGV->ltt.engaged);

        tft.setTextSize(1);
        for (int j = 0; j < 16; j++)
        {
            for (int i = 0; i < 16; i++)
            {
                const uint8_t val = pGV->ms.vetable[i + j * 16];
                if ((i == pGV->ltt.x[0] || i == pGV->ltt.x[1]) &&
                    (j == pGV->ltt.y[0] || j == pGV->ltt.y[1]))
                {
                    tft.setTextColor(ILI9341_BLACK, ILI9341_YELLOW);
                    // if (error > 1020) // 102%
                    // {
                    //     val = pGV->ms.vetable[i + j * 16] += 1;
                    //     pGV->ms.ve_updated = true;
                    // }
                    // else if (error < 980) // 98%
                    // {
                    //     val = pGV->ms.vetable[i + j * 16] -= 1;
                    //     pGV->ms.ve_updated = true;
                    // }
                }
                else if (val < 98)
                {
                    tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
                }
                else if (val > 102)
                {
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKCYAN);
                }
                else
                {
                    tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREEN);
                }

                tft.setCursor(20 * i, -10 * j + 230);
                if (val < 10) tft.print(' ');
                if (val < 100) tft.print(' ');
                tft.print(val);
            }
        }
        updateDisplay();
    }
}

int display_init(int prio, void* arg)
{
    chThdCreateStatic(waThdDisplay, sizeof(waThdDisplay), prio++, ThreadDisplay, arg);
    return 1;
}