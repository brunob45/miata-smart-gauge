/***************************************************
  This is our GFX example for the Adafruit ILI9341 Breakout and Shield
  ----> http://www.adafruit.com/products/1651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <Arduino.h>

#include "SPI.h"
#include "ILI9341_t3.h"
#include "font_Arial.h"

// #include "image.h"

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define SCREEN_W (320)
#define SCREEN_H (240)

#define MAIN_TEXT_SIZE (5)
#define SEC_TEXT_SIZE (3)
#define CHAR_LEN(size) ((size)*6)

#define X_OFFSET (20)
#define Y_RPM (50)
#define Y_LOAD (Y_RPM + (MAIN_TEXT_SIZE * 9))
#define Y_LAUNCH (SCREEN_H - SEC_TEXT_SIZE * 9)

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

const int scale_item_count = 6;
const int scale_rpm_min = 1000;
const int scale_rpm_max = 7000;
const int scale_rpm_warn = 6500;
const int scale_rpm_dang = 6900;
const int scale_x_offset1 = 24;
const int scale_x_offset2 = 8;
const int scale_rpm_per_pixel = (scale_rpm_max - scale_rpm_min) / (SCREEN_W - scale_x_offset1 - scale_x_offset2);
const int scale_total_width = SCREEN_W - scale_x_offset1 - scale_x_offset2;

const int scale_span = scale_rpm_max - scale_rpm_min + ((scale_x_offset1 + scale_x_offset2) * scale_rpm_per_pixel);
const int scale_min = scale_rpm_min - scale_rpm_per_pixel * scale_x_offset1;
// const int scale_item_width = SCREEN_W / scale_item_count;

inline void print_image()
{
#if defined(IMAGE_H)
    const uint16_t image_offset = 22;
    tft.fillRect(0, 0, SCREEN_W, image_offset, ILI9341_BLACK);
    tft.writeRect(0, image_offset, SCREEN_W, SCREEN_H - image_offset, (const uint16_t *)&gimp_image.pixel_data[0]);
    delay(1000);
#endif
}

void setup()
{
    Serial1.begin(115200);
    analogWrite(6, 32);

    tft.begin();
    // Note: you can now set the SPI speed to any value
    // the default value is 30Mhz, but most ILI9341 displays
    // can handle at least 60Mhz and as much as 100Mhz
    tft.setClock(100e6);
    tft.setRotation(1);

    print_image();

    tft.setTextSize(MAIN_TEXT_SIZE);
    tft.fillScreen(ILI9341_BLACK);

    tft.drawRect(X_OFFSET-7, Y_RPM-4, SCREEN_W - 2*X_OFFSET + 14, SCREEN_H - Y_RPM+2, ILI9341_DARKGREY);

    tft.setTextColor(ILI9341_GREENYELLOW);
    tft.setCursor(X_OFFSET, Y_RPM);
    tft.print("RPM");
    tft.setCursor(X_OFFSET, Y_LOAD);
    tft.print("MAP");
    tft.setTextSize(SEC_TEXT_SIZE);
    tft.setCursor(X_OFFSET, Y_LAUNCH);
    tft.print("LC");
    tft.setTextSize(SEC_TEXT_SIZE);
    tft.setCursor(SCREEN_W - X_OFFSET-CHAR_LEN(SEC_TEXT_SIZE) * 6, Y_LAUNCH);
    tft.print("FAN");

    const int x1 = SCREEN_W * (scale_rpm_warn - scale_min) / scale_span;
    const int x2 = SCREEN_W * (scale_rpm_dang - scale_min) / scale_span;

    tft.drawRect(x1, 25, x2 - x1, 2, ILI9341_ORANGE);
    tft.drawRect(x2, 25, SCREEN_W - x2, 2, ILI9341_RED);
    tft.setTextSize(2);
    for (int i = 0; i <= scale_item_count; i++)
    {
        uint16_t x = scale_x_offset1 + (scale_total_width * i / scale_item_count);
        tft.drawRect(x - 1, 25, 2, 2, ILI9341_WHITE);
        tft.setCursor(x - 5, 29);
        tft.print(i + 1);
    }
}

static bool get_rpm(int16_t *rpm, int16_t *load, uint8_t* status1)
{
    // Clear RX buffer before request
    while (Serial1.available())
    {
        Serial1.read();
    }

    const uint8_t command[] = {
        'r',  // Command
        0,    // ID
        1, 0, // Offset, little-endian
        9, 0, // Size, little-endian
    };
    Serial1.write(command, sizeof(command));

    uint32_t begin = millis();
    while (Serial1.available() < 9)
    {
        // Check for comm timeout
        if (millis() - begin >= 10)
            return false;
        else
            continue;
    }

    uint8_t a = Serial1.read(), b = Serial1.read();
    if (rpm)
    {
        *rpm = word(b, a);
    }
    a = Serial1.read();
    b = Serial1.read();
    if (load)
    {
        *load = word(b, a);
    }

    for(int i = 0; i < 4; i++)
    {
        Serial1.read(); // discard bytes
    }

    uint8_t flags = Serial1.read();
    if (status1)
    {
        *status1 = flags;
    }

    return true;
}

bool getLaunch(uint8_t *flags)
{
    // Clear RX buffer before request
    while (Serial1.available())
    {
        Serial1.read();
    }

    const uint8_t command[] = {
        'r',   // Command
        0,     // ID
        68, 0, // Offset, little-endian
        1, 0,  // Size, little-endian
    };
    Serial1.write(command, sizeof(command));

    uint32_t begin = millis();
    while (Serial1.available() < 1)
    {
        // Check for comm timeout
        if (millis() - begin >= 10)
            return false;
        else
            continue;
    }

    *flags = Serial1.read();

    return true;
}

static uint16_t rpmcur = SCREEN_W - X_OFFSET - CHAR_LEN(MAIN_TEXT_SIZE), loadcur = SCREEN_W - X_OFFSET - CHAR_LEN(MAIN_TEXT_SIZE);
static char rpmbuf[6], loadbuf[6];
static int16_t last_rpm = -1, last_load = -1;
static uint8_t last_flags = -1, last_status1 = -1;

void loop(void)
{
    static uint32_t last_update = 0;

    const uint32_t now = millis();
    if (now - last_update < 100)
    {
        return;
    }
    last_update = now;

    int16_t rpm = 0, load = 0;
    uint8_t status1 = false;

    get_rpm(&rpm, &load, &status1);

    if (rpm < 0 || rpm > 10000)
    {
        // invalid data received
        rpm = 0;
    }

    if (load < 0 || load > 2000) // 200.0 kPa
    {
        // invalid data received
        load = 0;
    }

    if (rpm != last_rpm)
    {
        const int n_digits = snprintf(rpmbuf, 6, "%5d", rpm);
        rpmcur = SCREEN_W - X_OFFSET + MAIN_TEXT_SIZE - (n_digits * CHAR_LEN(MAIN_TEXT_SIZE));

        tft.setTextSize(MAIN_TEXT_SIZE);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(rpmcur, Y_RPM);
        tft.print(rpmbuf);

        uint16_t rectw = rpm < scale_min ? 0 : SCREEN_W * (rpm - scale_min) / scale_span;

        tft.fillRect(0, 0, rectw, 24, ILI9341_GREENYELLOW);
        tft.fillRect(rectw + 1, 0, SCREEN_W - rectw, 24, ILI9341_BLACK);

        last_rpm = rpm;
    }

    if (load != last_load)
    {
        const int n_digits = snprintf(loadbuf, 6, "%3d.%d", load / 10, load % 10);
        loadcur = SCREEN_W - X_OFFSET + MAIN_TEXT_SIZE - (n_digits * CHAR_LEN(MAIN_TEXT_SIZE));

        tft.setTextSize(MAIN_TEXT_SIZE);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(loadcur, Y_LOAD);
        tft.print(loadbuf);

        last_load = load;
    }

    uint8_t flags = 0;
    getLaunch(&flags);

    if (flags != last_flags)
    {
        tft.setTextSize(SEC_TEXT_SIZE);
        tft.setCursor(X_OFFSET + SEC_TEXT_SIZE + (CHAR_LEN(SEC_TEXT_SIZE) * 2), Y_LAUNCH);

        if (flags & (1 << 5))
        {
            tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
            tft.print("CUT");
        }
        else if (flags & (1 << 4))
        {
            tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
            tft.print("RDY");
        }
        else
        {
            tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
            tft.print("OFF");
        }

        last_flags = flags;
    }

    if (status1 != last_status1)
    {
        tft.setTextSize(SEC_TEXT_SIZE);
        tft.setCursor(SCREEN_W - X_OFFSET - (CHAR_LEN(SEC_TEXT_SIZE) * 3) + SEC_TEXT_SIZE, Y_LAUNCH);

        if (status1 & (1<<4))
        {
            tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
            tft.print(" ON");
        }
        else
        {
            tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
            tft.print("OFF");
        }

        last_status1 = status1;
    }
}
