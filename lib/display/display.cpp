#include "display.h"

#include "git_sha.h"
#include "miata.h"
#include "point.h"

#define TFT_CS 10
#define TFT_DC 9

static uint16_t header_565_cmap[256];
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

namespace Display
{
namespace Internal
{
struct DisplayMenu
{
    void (*init)();
    void (*update)();
};

uint8_t numSize(uint16_t n);
void drawNumber(int number, int scale, int offset, int x, int y);

void initMenu0();
void initMenu1();
void initMenu2();

void updateMenu0();
void updateMenu1();
void updateMenu2();

DisplayMenu menus[] = {{initMenu0, updateMenu0}, {initMenu1, updateMenu1}, {initMenu2, updateMenu2}};
uint8_t current_menu = 2;
bool _isReady = false;
uint32_t _readyTime = 0;

DMAMEM uint16_t tft_frame_buffer0[240 * 320];
DMAMEM uint16_t tft_frame_buffer1[240 * 320];

ILI9341_t3n tft(TFT_CS, TFT_DC);

void switchFrameBuffer()
{
    if (tft.getFrameBuffer() == tft_frame_buffer0)
    {
        tft.setFrameBuffer(tft_frame_buffer1);
    }
    else
    {
        tft.setFrameBuffer(tft_frame_buffer0);
    }
}

} // namespace Internal

using namespace Internal;

void init(void)
{
    get565cmap();

    tft.begin(60e6);
    switchFrameBuffer();
    //tft.setFrameBuffer(fb);
    tft.useFrameBuffer(true);
    tft.setRotation(3);

    tft.writeRect8BPP(0, 0, width, height, header_data, header_565_cmap);
    tft.updateScreenAsync();
}

void initFB()
{
    menus[current_menu].init();
    tft.setCursor(45, 5);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    tft.print(GIT_SHA);
}

void update(void)
{
    static bool initDone = false;
    static uint32_t last_update = 0;
    static uint32_t last_delta = 0;
    static size_t line_max = 0;

    const uint32_t now = millis();

    if (!_isReady)
    {
        if (!tft.asyncUpdateActive())
        {
            _isReady = true;
            _readyTime = now;

            // Init both Frame buffers.
            initFB();
            switchFrameBuffer();
            initFB();
        }
        else
        {
            return;
        }
    }

    initDone |= (now - _readyTime) > 2000;

    if (tft.asyncUpdateActive())
    {
        return;
    }

    if (initDone)
    {
        tft.updateScreenAsync();
//        switchFrameBuffer();
        last_delta = now - last_update;
        last_update = now;
    }

    menus[current_menu].update();

    tft.setCursor(5, 240 - 10);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    size_t line_len = tft.print(last_delta);
    line_len += tft.print("ms, ");

    uint32_t seconds = now / 1000;
    if (seconds >= 3600)
    {
        uint16_t hours = seconds / 3600;
        seconds = seconds % 3600;
        line_len += tft.print(hours);
        line_len += tft.print("h");
        if (seconds < 600)
        {
            line_len += tft.print(' ');
        }
    }
    if (seconds >= 60)
    {
        uint16_t minutes = seconds / 60;
        seconds = seconds % 60;
        line_len += tft.print(minutes);
        line_len += tft.print("m");
        if (seconds < 10)
        {
            line_len += tft.print(' ');
        }
    }
    line_len += tft.print(seconds);
    line_len += tft.print("s");

    // clear line
    line_max = max(line_max, line_len);
    for (size_t i = line_len; i < line_max; i++)
    {
        tft.print(' ');
    }
}

void alert(bool enable)
{
}

bool isReady()
{
    return _isReady;
}

} // namespace Display