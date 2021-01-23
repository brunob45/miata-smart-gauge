#include "main.h"

#include <ILI9341_t3.h>

#define TFT_DC 9
#define TFT_CS 10

#define DISPLAY_ALERT ILI9341_RED
#define DISPLAY_ACCENT1 0x7fe2
#define DISPLAY_ACCENT2 0x39e7
#define DISPLAY_FG1 ILI9341_LIGHTGREY
#define DISPLAY_FG2 ILI9341_DARKGREY
#define DISPLAY_BG ILI9341_BLACK

namespace
{
typedef union
{
    struct
    {
        int16_t x;
        int16_t y;
    } cart;
    struct pol
    {
        int16_t a;
        int16_t r;
    } pol;
} Coord;

void initMenu1();
void initMenu2();

void updateMenu1();
void updateMenu2();

uint8_t numSize(uint16_t n);

ILI9341_t3 tft(TFT_CS, TFT_DC);

Coord gaugeCenter = {.cart = {.x = 320 - 52, 240 - 82}};
int16_t gaugeRadius = 140;
uint16_t GAUGE_BG = DISPLAY_BG;
bool update_bg = false;

struct DisplayMenu
{
    void (*init)();
    void (*update)();
};
DisplayMenu menus[] = {{initMenu1, updateMenu1}, {initMenu2, updateMenu2}};

Coord pol2cart(int16_t a, int16_t r)
{
    Coord c;
    c.cart.x = sin(radians(a)) * r;
    c.cart.y = cos(radians(a)) * r;
    return c;
}
} // namespace

DisplayClass::DisplayClass(int pin_lumi)
    : _current_menu(0), _pin_lumi(pin_lumi)
{
}

void DisplayClass::init()
{
    pinMode(_pin_lumi, OUTPUT);
    setLumi(50);

    tft.begin();
    // Note: you can now set the SPI speed to any value
    // the default value is 30Mhz, but most ILI9341 displays
    // can handle at least 60Mhz and as much as 100Mhz
    tft.setClock(60e6);
    tft.setRotation(3);

    menus[_current_menu].init();
}

void DisplayClass::setLumi(uint16_t lumi)
{
    analogWrite(_pin_lumi, (lumi * lumi) / 100U);
}

void DisplayClass::update()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    menus[_current_menu].update();

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

namespace
{
void initGauge()
{
    tft.fillCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius + 12, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius + 11, GAUGE_BG);
    tft.fillCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius - 12, GAUGE_BG);
    tft.drawCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius - 12, DISPLAY_ACCENT1);

    tft.setTextSize(2);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setTextColor(0x7fe2);

    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Coord c = pol2cart(a, gaugeRadius);
        tft.setCursor(gaugeCenter.cart.x - c.cart.x - 4, gaugeCenter.cart.y - c.cart.y - 7);
        tft.print(8 - i);
        a += 17;
    }
}

void initMenu1()
{
    tft.fillScreen(DISPLAY_BG);

    initGauge();

    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setCursor(5, 75);
    tft.print("PW");
    tft.setCursor(5, 75 + 50);
    tft.print("MAP");
    tft.setCursor(5, 75 + 100);
    tft.print("a");
}

void initMenu2()
{
    tft.fillScreen(DISPLAY_BG);
    initGauge();
}

void updateGauge()
{
    static Coord needle[2];
    static bool wasAlert = false;

    if (GV.alert != wasAlert)
    {
        wasAlert = GV.alert;
        GAUGE_BG = wasAlert ? DISPLAY_ALERT : DISPLAY_BG;
        tft.fillCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius - 14, GAUGE_BG);
        update_bg = false;
    }

    Coord c1 = pol2cart(-(GV.rpm * 17 / 1000) + 119, gaugeRadius - 16);
    Coord c2 = pol2cart(-(GV.rpm * 17 / 1000) + 119, -30);

    tft.drawLine(needle[0].cart.x, needle[0].cart.y, needle[1].cart.x, needle[1].cart.y, GAUGE_BG);
    tft.drawLine(needle[0].cart.x + 1, needle[0].cart.y, needle[1].cart.x + 1, needle[1].cart.y, GAUGE_BG);
    tft.drawLine(needle[0].cart.x, needle[0].cart.y + 1, needle[1].cart.x, needle[1].cart.y + 1, GAUGE_BG);

    needle[0].cart.x = gaugeCenter.cart.x - c2.cart.x;
    needle[0].cart.y = gaugeCenter.cart.y - c2.cart.y;
    needle[1].cart.x = gaugeCenter.cart.x - c1.cart.x;
    needle[1].cart.y = gaugeCenter.cart.y - c1.cart.y;

    tft.drawLine(needle[0].cart.x, needle[0].cart.y, needle[1].cart.x, needle[1].cart.y, DISPLAY_FG1);
    tft.drawLine(needle[0].cart.x + 1, needle[0].cart.y, needle[1].cart.x + 1, needle[1].cart.y, DISPLAY_FG1);
    tft.drawLine(needle[0].cart.x, needle[0].cart.y + 1, needle[1].cart.x, needle[1].cart.y + 1, DISPLAY_FG1);

    tft.drawCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, 9, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, 8, DISPLAY_ACCENT1);

    tft.setTextSize(4);
    tft.setTextColor(DISPLAY_FG1, GAUGE_BG);
    tft.setCursor(220, 205);

    for (int i = 0; i < 4 - numSize(GV.rpm); i++)
    {
        tft.print(' ');
    }
    tft.print(GV.rpm);
}

void updateMenu1()
{
    updateGauge();

    tft.setTextSize(3);
    tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);

    tft.setCursor(5, 92 + 100);
    tft.print(abs(GV.acc_s - 1) * 9.8);
    tft.setCursor(5, 92 + 50);
    tft.print("99.9");
    tft.setCursor(5, 92);
    tft.print("12.678");
}

void updateMenu2()
{
    updateGauge();

    tft.setTextSize(10);
    tft.setCursor(30, 100);

    if (GV.current_gear > 0)
    {
        tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
        tft.print(GV.current_gear);
    }
    else
    {
        tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
        tft.print('N');
    }
}

uint8_t numSize(uint16_t n)
{
    uint8_t s = 0;
    do
    {
        n /= 10;
        s++;
    } while (n > 0);

    return s;
}
} // namespace