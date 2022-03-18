#include "display.h"

#include "bg.c"
#include "git_sha.h"
#include "miata.h"
#include "point.h"

#include <lvgl.h>

#define TFT_CS 10
#define TFT_DC 9

// static uint16_t header_565_cmap[256];
// constexpr void get565cmap()
// {
//     for (uint16_t i = 0; i < 256; i++)
//     {
//         const uint8_t r = header_data_cmap[i][0];
//         const uint8_t g = header_data_cmap[i][1];
//         const uint8_t b = header_data_cmap[i][2];
//         header_565_cmap[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
//     }
// }

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

ILI9341_t3n tft(TFT_CS, TFT_DC);
} // namespace Internal

using namespace Internal;

IntervalTimer it;
IntervalTimer it2;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[240 * 320 / 10];
static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
static lv_obj_t* label;
static lv_obj_t* gauge;
static lv_style_t style_text;

void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
    for (int y = area->y1; y <= area->y2; y++)
    {
        for (int x = area->x1; x <= area->x2; x++)
        {
            tft.drawPixel(x, y, color_p->full);
            color_p++;
        }
    }

    lv_disp_flush_ready(disp);
    // tft.writeRect(area->x1, area->y1, area->x2 - area->x1, area->y2 - area->y1, (uint16_t*)color_p);
}

void update_cpt()
{
    static int cpt;
    lv_timer_handler();
    lv_label_set_text_fmt(label, "%d", cpt++);
}

void init(void)
{
    tft.begin(60e6);
    tft.setRotation(3);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);
    tft.updateScreenAsync(true);

    tft.fillScreen(ILI9341_BLUE);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 240 * 320 / 10);

    lv_disp_drv_init(&disp_drv);       /*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush; /*Set your driver function*/
    disp_drv.draw_buf = &draw_buf;     /*Assign the buffer to the display*/
    disp_drv.hor_res = 320;            /*Set the horizontal resolution of the display*/
    disp_drv.ver_res = 240;            /*Set the vertical resolution of the display*/
    lv_disp_drv_register(&disp_drv);   /*Finally register the driver*/

    // get565cmap();

    lv_obj_t* obj = lv_img_create(lv_scr_act());

    lv_img_set_src(obj, &splash_screen);
    lv_obj_set_align(obj, LV_ALIGN_TOP_LEFT);

    label = lv_label_create(lv_scr_act());
    lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);

    gauge = lv_meter_create(lv_scr_act());
    lv_obj_set_align(gauge, LV_ALIGN_TOP_RIGHT);

    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, {.ch = {.blue = 0x1f, .green = 0x3f, .red = 0x1f}});
    lv_style_set_text_font(&style_text, &lv_font_montserrat_28);
    lv_obj_add_style(label, &style_text, LV_PART_MAIN | LV_STATE_DEFAULT);

    // tft.writeRect8BPP(0, 0, width, height, header_data, header_565_cmap);
    // tft.updateScreenAsync();

    it.begin([]()
             { lv_tick_inc(1); },
             1000);
    it2.begin(update_cpt, 20000);
}

void update(void)
{
    _isReady = true;
    return;

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

            menus[current_menu].init();

            tft.setCursor(45, 5);
            tft.setTextSize(1);
            tft.setTextColor(DISPLAY_FG2, DISPLAY_BG);

            tft.print(GIT_SHA);
        }
        else
        {
            return;
        }
    }

    initDone |= (now - _readyTime) > 2000;

    if (initDone)
    {
        if (tft.asyncUpdateActive())
            return;

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
        tft.updateScreenAsync();
        last_delta = now - last_update;
        last_update = now;
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