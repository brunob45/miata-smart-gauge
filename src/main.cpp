
#include <ILI9341_t3.h>
#include <Adafruit_MSA301.h>

#include "main.h"

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

#define TFT_DC 9
#define TFT_CS 10

#define DISPLAY_ALERT ILI9341_RED;
#define DISPLAY_ACCENT1 0x7fe2
#define DISPLAY_ACCENT2 0x39e7
#define DISPLAY_FG1 ILI9341_LIGHTGREY
#define DISPLAY_FG2 ILI9341_DARKGREY
#define DISPLAY_BG ILI9341_BLACK

ILI9341_t3 tft(TFT_CS, TFT_DC);
Adafruit_MSA301 msa;

Coord gaugeCenter = {.cart = {.x = 320 - 52, 240 - 82}};
int16_t gaugeRadius = 140;

uint16_t GAUGE_BG = DISPLAY_BG;
bool alert = false;
bool update_bg = false;

float acc_x = 0;
float acc_y = 0;
float acc_z = 0;
float acc_s = 0;

int16_t rpm = 0;
int16_t rpm_inc = 100;
uint8_t current_gear = 0;

uint8_t LinLum[] = {1,5,21,47,84,130,188,255};

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

    msa.begin();

    menus[current_menu].init();

    commTimer.begin(update_comms, 1000); // Do not increase above 5000us
}

void loop(void)
{
    update_accel();
    update_values();
    update_display();
}

void print_accel_params(Adafruit_MSA301 &a, Print &p)
{
    // a.setDataRate(MSA301_DATARATE_125_HZ);
    p.print("Data rate set to: ");
    switch (a.getDataRate())
    {
    case MSA301_DATARATE_1_HZ:
        p.println("1 Hz");
        break;
    case MSA301_DATARATE_1_95_HZ:
        p.println("1.95 Hz");
        break;
    case MSA301_DATARATE_3_9_HZ:
        p.println("3.9 Hz");
        break;
    case MSA301_DATARATE_7_81_HZ:
        p.println("7.81 Hz");
        break;
    case MSA301_DATARATE_15_63_HZ:
        p.println("15.63 Hz");
        break;
    case MSA301_DATARATE_31_25_HZ:
        p.println("31.25 Hz");
        break;
    case MSA301_DATARATE_62_5_HZ:
        p.println("62.5 Hz");
        break;
    case MSA301_DATARATE_125_HZ:
        p.println("125 Hz");
        break;
    case MSA301_DATARATE_250_HZ:
        p.println("250 Hz");
        break;
    case MSA301_DATARATE_500_HZ:
        p.println("500 Hz");
        break;
    case MSA301_DATARATE_1000_HZ:
        p.println("1000 Hz");
        break;
    }

    //a.setPowerMode(MSA301_SUSPENDMODE);
    p.print("Power mode set to: ");
    switch (a.getPowerMode())
    {
    case MSA301_NORMALMODE:
        p.println("Normal");
        break;
    case MSA301_LOWPOWERMODE:
        p.println("Low Power");
        break;
    case MSA301_SUSPENDMODE:
        p.println("Suspend");
        break;
    }

    //a.setBandwidth(MSA301_BANDWIDTH_31_25_HZ);
    p.print("Bandwidth set to: ");
    switch (a.getBandwidth())
    {
    case MSA301_BANDWIDTH_1_95_HZ:
        p.println("1.95 Hz");
        break;
    case MSA301_BANDWIDTH_3_9_HZ:
        p.println("3.9 Hz");
        break;
    case MSA301_BANDWIDTH_7_81_HZ:
        p.println("7.81 Hz");
        break;
    case MSA301_BANDWIDTH_15_63_HZ:
        p.println("15.63 Hz");
        break;
    case MSA301_BANDWIDTH_31_25_HZ:
        p.println("31.25 Hz");
        break;
    case MSA301_BANDWIDTH_62_5_HZ:
        p.println("62.5 Hz");
        break;
    case MSA301_BANDWIDTH_125_HZ:
        p.println("125 Hz");
        break;
    case MSA301_BANDWIDTH_250_HZ:
        p.println("250 Hz");
        break;
    case MSA301_BANDWIDTH_500_HZ:
        p.println("500 Hz");
        break;
    }

    //a.setRange(MSA301_RANGE_2_G);
    p.print("Range set to: ");
    switch (a.getRange())
    {
    case MSA301_RANGE_2_G:
        p.println("+-2G");
        break;
    case MSA301_RANGE_4_G:
        p.println("+-4G");
        break;
    case MSA301_RANGE_8_G:
        p.println("+-8G");
        break;
    case MSA301_RANGE_16_G:
        p.println("+-16G");
        break;
    }

    //a.setResolution(MSA301_RESOLUTION_14 );
    p.print("Resolution set to: ");
    switch (a.getResolution())
    {
    case MSA301_RESOLUTION_14:
        p.println("14 bits");
        break;
    case MSA301_RESOLUTION_12:
        p.println("12 bits");
        break;
    case MSA301_RESOLUTION_10:
        p.println("10 bits");
        break;
    case MSA301_RESOLUTION_8:
        p.println("8 bits");
        break;
    }
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

void update_accel()
{
    static uint32_t last_update = 0;
    const uint32_t now = millis();

    if (now - last_update < 20)
        return;

    msa.read();

    acc_x += (msa.x_g - acc_x) / 8;
    acc_y += (msa.y_g - acc_y) / 8;
    acc_z += (msa.z_g - acc_z) / 8;

    acc_s = acc_x * acc_x;
    acc_s += acc_y * acc_y;
    acc_s += acc_z * acc_z;
    acc_s = sqrtf(acc_s);

    last_update = now;
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

    current_gear = rpm/1335;

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

Coord pol2cart(int16_t a, int16_t r)
{
    Coord c;
    c.cart.x = sin(radians(a)) * r;
    c.cart.y = cos(radians(a)) * r;
    return c;
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

    if (update_bg)
    {
        tft.fillCircle(gaugeCenter.cart.x, gaugeCenter.cart.y, gaugeRadius - 14, GAUGE_BG);
        update_bg = false;
    }

    Coord c1 = pol2cart(-(rpm * 17 / 1000) + 119, gaugeRadius - 16);
    Coord c2 = pol2cart(-(rpm * 17 / 1000) + 119, -30);

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
    int cursor_x = (offset-whole_size) * 18 + x;
    tft.setCursor(cursor_x, y);
    tft.print(whole);
    cursor_x += whole_size * 18 + 6;
    tft.setCursor(cursor_x, y);
    tft.print(decimal);
    cursor_x = offset * 18 + x - 6;
    tft.setTextColor(DISPLAY_FG1 , DISPLAY_FG1);
    tft.setCursor(cursor_x, y);
    tft.print('.');
}

void updateMenu1()
{
    const int map = 1022;
    updateGauge();

    drawNumber(abs(acc_s-1)*980, 100, 1, 5, 92+100);
    drawNumber(map, 10, 3, 5, 92+50);
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