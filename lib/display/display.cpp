#include "display.h"

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

void initMenu1();
void initMenu2();

void updateMenu1();
void updateMenu2();

DisplayMenu menus[] = {{initMenu1, updateMenu1}, {initMenu2, updateMenu2}};
uint8_t current_menu = 0;

ILI9341_t3 tft(TFT_CS, TFT_DC);
} // namespace Internal

using namespace Internal;

uint8_t lumi(uint8_t percent)
{
    unsigned int l = (percent * 255u) / 100u;
    return (l * l) / 256u;
}

void init(void)
{
    pinMode(6, OUTPUT);
    analogWrite(6, lumi(50));
    tft.begin();
    tft.setClock(60e6);
    tft.setRotation(3);
    menus[current_menu].init();
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
    uint16_t seconds = now / 1000;
    if (seconds >= 60)
    {
        uint16_t minutes = seconds / 60;
        seconds = seconds % 60;
        tft.print(minutes);
        tft.print("m");
    }
    tft.print(seconds);
    tft.print("s   ");

    last_update = now;
}

void alert(bool enable)
{
}

} // namespace Display