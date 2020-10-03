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

#define CHAR_WIDTH (6)
#define TEXT_SIZE (6)
#define TEXT_LENGTH (8)
#define CHAR_LEN ((CHAR_WIDTH) * (TEXT_SIZE))

const uint16_t X_OFFSET = (SCREEN_W - (CHAR_LEN * TEXT_LENGTH)) / 2;
const uint16_t Y_RPM = 40;
const uint16_t Y_LOAD = Y_RPM + 60;

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

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

    tft.begin();
    // Note: you can now set the SPI speed to any value
    // the default value is 30Mhz, but most ILI9341 displays
    // can handle at least 60Mhz and as much as 100Mhz
    tft.setClock(100e6);
    tft.setRotation(1);

    print_image();

    tft.setTextSize(TEXT_SIZE);
    tft.fillScreen(ILI9341_BLACK);
    // tft.fillRect(0, 0, SCREEN_W, (6 * 7) + (Y_OFFSET * 2), ILI9341_BLACK);

    tft.setCursor(X_OFFSET, Y_RPM);
    tft.print("RPM");
    tft.setCursor(X_OFFSET, Y_LOAD);
    tft.print("MAP");
}

static bool get_rpm(int16_t* rpm, int16_t* load)
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
        4, 0, // Size, little-endian
    };
    Serial1.write(command, sizeof(command));

    uint32_t begin = millis();
    while (Serial1.available() < 2)
    {
        // Check for comm timeout
        if (millis() - begin >= 100)
            return false;
        else
            continue;
    }

    uint8_t a = Serial1.read(), b = Serial1.read();
    if (rpm)
    {
        *rpm = word(b, a);
    }
    a = Serial1.read(); b = Serial1.read();
    if (load)
    {
        *load = word(b, a);
    }

    return true;
}

static uint16_t rpmcur = SCREEN_W - X_OFFSET - CHAR_LEN, loadcur = SCREEN_W - X_OFFSET - CHAR_LEN;
static char rpmbuf[8], loadbuf[8];
static int16_t last_rpm = -1, last_load = -1;

void loop(void)
{
    static uint32_t last_update = 0;

    const uint32_t now = millis();
    if (now - last_update < 100)
    {
        return;
    }
    last_update = now;

    int16_t rpm, load;

    if(!get_rpm(&rpm, &load))
    {
        rpm = 0;
        load = 0;
    }

    if (rpm != last_rpm)
    {
        tft.setTextColor(ILI9341_BLACK);
        tft.setCursor(rpmcur, Y_RPM);
        tft.print(rpmbuf);

        const uint8_t n_digits = sprintf(rpmbuf, "%d", rpm);
        rpmcur = SCREEN_W - X_OFFSET - (n_digits * CHAR_LEN);

        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(rpmcur, Y_RPM);
        tft.print(rpmbuf);

        uint16_t rectw = SCREEN_W * rpm / 7000;

        tft.fillRect(0, 0, rectw, 30, ILI9341_WHITE);
        tft.fillRect(rectw+1, 0, SCREEN_W - rectw, 30, ILI9341_BLACK);

        last_rpm = rpm;
    }

    if (load != last_load)
    {
        tft.setTextColor(ILI9341_BLACK);
        tft.setCursor(loadcur, Y_LOAD);
        tft.print(loadbuf);

        const uint8_t n_digits = sprintf(loadbuf, "%.1f", load/10.0f);
        loadcur = SCREEN_W - X_OFFSET - (n_digits * CHAR_LEN);

        tft.setTextColor(ILI9341_WHITE);
        tft.setCursor(loadcur, Y_LOAD);
        tft.print(loadbuf);
    
        last_load = load;
    }
}
