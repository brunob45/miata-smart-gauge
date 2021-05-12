#include "display.h"

#include "git_sha.h"
#include "point.h"

#define TFT_DC 9
#define TFT_CS 10

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

DisplayMenu menus[] = {{initMenu1, updateMenu1}, {initMenu2, updateMenu2}, {initMenu3, updateMenu3}};
uint8_t current_menu = 2;

ILI9341_t3 tft(TFT_CS, TFT_DC);
} // namespace Internal

using namespace Internal;

void init(void)
{
    tft.begin();
    tft.setClock(60e6);
    tft.setRotation(3);
    tft.fillScreen(ILI9341_WHITE);
    menus[current_menu].init();

    tft.setCursor(45, 5);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    tft.print(GIT_SHA);
}

void update(void)
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    menus[current_menu].update();

    tft.setCursor(5, 240 - 10);
    tft.setTextSize(1);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

    tft.print(millis() - now);
    tft.print("ms, ");

    uint32_t seconds = now / 1000;
    if (seconds >= 3600)
    {
        uint16_t hours = seconds / 3600;
        seconds = seconds % 3600;
        tft.print(hours);
        tft.print("h");
        if (seconds < 600)
        {
            tft.print(' ');
        }
    }
    if (seconds >= 60)
    {
        uint16_t minutes = seconds / 60;
        seconds = seconds % 60;
        tft.print(minutes);
        tft.print("m");
        if (seconds < 10)
        {
            tft.print(' ');
        }
    }
    tft.print(seconds);
    tft.print("s");

    last_update = now;
}

void alert(bool enable)
{
}

} // namespace Display