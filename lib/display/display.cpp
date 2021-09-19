#include "display.h"

#include "git_sha.h"
#include "point.h"

#include <tgx.h>

#define TFT_SCK 13
#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_CS 10
#define TFT_DC 9

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

ILI9341_T4::DiffBuffStatic<6000> diff1;
ILI9341_T4::DiffBuffStatic<6000> diff2;

ILI9341_T4::ILI9341Driver tft(TFT_CS, TFT_DC, TFT_SCK, TFT_MOSI, TFT_MISO);

uint16_t internal_fb[320 * 240];
uint16_t fb[320 * 240];
} // namespace Internal

using namespace Internal;

void init(void)
{
    tft.begin(60e6);
    tft.setRotation(3);
    menus[current_menu].init();

    tft.setCursor(45, 5);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    tft.print(GIT_SHA);
}

void update(void)
{
    static uint32_t last_update = 0;
    static size_t line_max = 0;

    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    menus[current_menu].update();

    tft.setCursor(5, 240 - 10);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    size_t line_len = tft.print(millis() - now);
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

    last_update = now;
}

void alert(bool enable)
{
}

} // namespace Display