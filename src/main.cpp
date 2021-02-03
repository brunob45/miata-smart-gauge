
#include "main.h"

#include <ILI9341_t3.h>

#include "point.h"

#define TFT_DC 9
#define TFT_CS 10

#define DISPLAY_ALERT ILI9341_RED;
#define DISPLAY_ACCENT1 0x7fe2
#define DISPLAY_ACCENT2 0x39e7
#define DISPLAY_FG1 ILI9341_LIGHTGREY
#define DISPLAY_FG2 ILI9341_DARKGREY
#define DISPLAY_BG ILI9341_BLACK

ILI9341_t3 tft(TFT_CS, TFT_DC);

Point gaugeCenter(320 - 52, 240 - 82, Point::CARTESIAN);
int16_t gaugeRadius = 140;

uint16_t GAUGE_BG = DISPLAY_BG;
bool alert = false;
bool update_bg = false;

int16_t rpm = 0;
int16_t rpm_inc = 100;
uint8_t current_gear = 0;

uint8_t LinLum[] = {1, 5, 21, 47, 84, 130, 188, 255};

struct DisplayMenu
{
    void (*init)();
    void (*update)();
};

void initMenu1();
void initMenu2();

void updateMenu1();
void updateMenu2();

DisplayMenu menus[] = {{initMenu1, updateMenu1}, {initMenu2, updateMenu2}};
uint8_t current_menu = 0;

IntervalTimer commTimer;

void setup()
{
    pinMode(6, OUTPUT);
    analogWrite(6, LinLum[5]);

    Serial.begin(115200);
    Serial1.begin(115200);

    tft.begin();
    // Note: you can now set the SPI speed to any value
    // the default value is 30Mhz, but most ILI9341 displays
    // can handle at least 60Mhz and as much as 100Mhz
    tft.setClock(60e6);
    tft.setRotation(3);

    Accel::init();

    menus[current_menu].init();

    commTimer.begin(update_comms, 1000); // Do not increase above 5000us
}

void loop(void)
{
    Accel::update();
    update_values();
    update_display();
}

void update_comms(void)
{
    for (int xfer_size = Serial.available(); xfer_size > 0; xfer_size--)
    {
        Serial1.write(Serial.read());
    }
    for (int xfer_size = Serial1.available(); xfer_size > 0; xfer_size--)
    {
        Serial.write(Serial1.read());
    }
}

void update_values()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update < 100)
        return;

    rpm += rpm_inc;
    if (rpm >= 8000 && rpm_inc > 0)
    {
        rpm_inc = -200;
    }
    else if (rpm <= 200 && rpm_inc < 0)
    {
        rpm_inc = 100;
    }

    current_gear = rpm / 1335;

    if (rpm > 7200 && !alert)
    {
        GAUGE_BG = DISPLAY_ALERT;
        alert = true;
        update_bg = true;
    }
    else if (rpm < 7200 && alert)
    {
        GAUGE_BG = DISPLAY_BG;
        alert = false;
        update_bg = true;
    }

    last_update = now;
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

void update_display()
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

void initGauge()
{
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 12, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius + 11, GAUGE_BG);
    tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, GAUGE_BG);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 12, DISPLAY_ACCENT1);

    tft.setTextSize(2);
    tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
    tft.setTextColor(0x7fe2);

    int16_t a = -17;
    for (int i = 0; i < 9; i++)
    {
        Point c = gaugeCenter - Point(a, gaugeRadius, Point::POLAR);
        tft.setCursor(c.x() - 4, c.y() - 7);
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
    static Point needle[2];

    if (update_bg)
    {
        tft.fillCircle(gaugeCenter.x(), gaugeCenter.y(), gaugeRadius - 14, GAUGE_BG);
        update_bg = false;
    }

    Point c1(-(rpm * 17 / 1000) + 119, gaugeRadius - 16, Point::POLAR);
    Point c2(-(rpm * 17 / 1000) + 119, -30, Point::POLAR);

    tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), GAUGE_BG);
    tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), GAUGE_BG);
    tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, GAUGE_BG);

    needle[0] = gaugeCenter - c2;
    needle[1] = gaugeCenter - c1;

    tft.drawLine(needle[0].x(), needle[0].y(), needle[1].x(), needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x() + 1, needle[0].y(), needle[1].x() + 1, needle[1].y(), DISPLAY_FG1);
    tft.drawLine(needle[0].x(), needle[0].y() + 1, needle[1].x(), needle[1].y() + 1, DISPLAY_FG1);

    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 9, DISPLAY_ACCENT2);
    tft.drawCircle(gaugeCenter.x(), gaugeCenter.y(), 8, DISPLAY_ACCENT1);

    tft.setTextSize(4);
    tft.setTextColor(DISPLAY_FG1, GAUGE_BG);
    tft.setCursor(220, 205);

    for (int i = 0; i < 4 - numSize(rpm); i++)
    {
        tft.print(' ');
    }
    tft.print(rpm);
}

void drawNumber(int number, int scale, int offset, int x, int y)
{
    tft.setTextSize(3);
    tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);

    int whole = number / scale, decimal = number % scale;
    int whole_size = numSize(whole);
    int cursor_x = (offset - whole_size) * 18 + x;
    tft.setCursor(cursor_x, y);
    tft.print(whole);
    cursor_x += whole_size * 18 + 6;
    tft.setCursor(cursor_x, y);
    tft.print(decimal);
    cursor_x = offset * 18 + x - 6;
    tft.setTextColor(DISPLAY_FG1, DISPLAY_FG1);
    tft.setCursor(cursor_x, y);
    tft.print('.');
}

void updateMenu1()
{
    updateGauge();

    drawNumber(abs(Accel::get().norm() - 1) * 980, 100, 1, 5, 92 + 100);
    drawNumber(1022, 10, 3, 5, 92 + 50);
    drawNumber(12678, 1000, 2, 5, 92);
}

void updateMenu2()
{
    updateGauge();

    tft.setTextSize(10);
    tft.setCursor(30, 100);

    if (current_gear > 0)
    {
        tft.setTextColor(DISPLAY_FG1, DISPLAY_BG);
        tft.print(current_gear);
    }
    else
    {
        tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);
        tft.print('?');
    }
}