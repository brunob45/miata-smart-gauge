#include "display.h"

#include <ILI9341_t3n.h>
#include <lvgl.h>

#include "accel.h"

#define GET_UNUSED_STACK(wa) (chUnusedThreadStack(wa, sizeof(wa)))
#define GET_USED_STACK(wa) (sizeof(wa) - GET_UNUSED_STACK(wa))

namespace Display
{
namespace
{
THD_WORKING_AREA(waThdLVGL, 10 * 256);
THD_WORKING_AREA(waThdTick, 1 * 256);
THD_WORKING_AREA(waThdLabel, 5 * 256);

thread_t* pThdLabel;

const uint8_t TFT_CS = 10;
const uint8_t TFT_DC = 9;

ILI9341_t3n tft(TFT_CS, TFT_DC);
DMAMEM uint16_t tft_frame_buffer0[240 * 320];

const size_t lv_buf_size = 240 * 320 / 10;
lv_disp_draw_buf_t draw_buf;
lv_color_t buf1[lv_buf_size];
lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
    while (tft.asyncUpdateActive())
    {
        // wait for previous flush to finish
        chThdSleepMicroseconds(100);
    }

    for (int y = area->y1; y <= area->y2; y++)
    {
        for (int x = area->x1; x <= area->x2; x++)
        {
            tft.drawPixel(x, y, color_p->full);
            color_p++;
        }
    }

    tft.updateScreenAsync(false);
    lv_disp_flush_ready(disp);
}

THD_FUNCTION(ThreadLVGL, arg)
{
    (void)arg;

    tft.begin(60e6);
    tft.setRotation(3);
    tft.setFrameBuffer(tft_frame_buffer0);
    tft.useFrameBuffer(true);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, lv_buf_size);

    lv_disp_drv_init(&disp_drv);       /*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush; /*Set your driver function*/
    disp_drv.draw_buf = &draw_buf;     /*Assign the buffer to the display*/
    disp_drv.hor_res = 320;            /*Set the horizontal resolution of the display*/
    disp_drv.ver_res = 240;            /*Set the vertical resolution of the display*/
    lv_disp_drv_register(&disp_drv);   /*Finally register the driver*/

    pinMode(6, OUTPUT);
    digitalWrite(6, HIGH);

    chEvtSignal(pThdLabel, EVENT_MASK(0));

    for (;;)
    {
        lv_timer_handler();
        chThdSleepMilliseconds(20);
    }
}

THD_FUNCTION(ThreadTick, arg)
{
    for (;;)
    {
        lv_tick_inc(1);
        chThdSleepMilliseconds(1);
    }
}

THD_FUNCTION(ThreadLabel, arg)
{
    pThdLabel = chThdGetSelfX();
    chEvtWaitAny(ALL_EVENTS);

    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    lv_style_t style_main;
    lv_style_init(&style_main);
    lv_style_set_text_font(&style_main, &din1451_18);
    lv_style_set_bg_color(&style_main, lv_color_black());
    lv_style_set_text_color(&style_main, lv_color_white());
    lv_obj_add_style(lv_scr_act(), &style_main, LV_STATE_DEFAULT);

    lv_style_t style_chart;
    lv_style_init(&style_chart);
    lv_style_set_border_color(&style_chart, lv_color_white());
    lv_style_set_border_width(&style_chart, 1);

    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);

    lv_obj_t* meter = lv_meter_create(lv_scr_act());
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_set_pos(meter, 160, 20);
    lv_obj_set_size(meter, 220, 220);

    lv_meter_scale_t* scale = lv_meter_add_scale(meter);
    const int rot = 110;
    lv_meter_set_scale_range(meter, scale, 0, 80, (270 - rot) * 8 / 7, rot);
    lv_meter_set_scale_ticks(meter, scale, 8 * 4 + 1, 2, 10, lv_palette_main(LV_PALETTE_LIME));
    lv_meter_set_scale_major_ticks(meter, scale, 4, 4, 15, lv_color_white(), 15);

    lv_meter_indicator_t* indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_LIME), -10);

    lv_obj_t* label_rpm = lv_label_create(lv_scr_act());
    lv_obj_align(label_rpm, LV_ALIGN_BOTTOM_RIGHT, -20, 0);

    lv_obj_t* chart = lv_chart_create(lv_scr_act());
    lv_obj_remove_style(chart, NULL, LV_PART_MAIN);
    lv_obj_add_style(chart, &style_chart, LV_PART_MAIN);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_size(chart, 145, 50);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -100, 100);
    lv_chart_set_point_count(chart, 145);

    lv_chart_series_t* serie1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_LIME), LV_CHART_AXIS_PRIMARY_Y);

    int dir = 1;
    int cpt = 0;

    for (;;)
    {
        uint16_t rpm = cpt * 100;

        lv_meter_set_indicator_value(meter, indic, cpt);

        lv_label_set_text_fmt(label_rpm, "%u", rpm);

        lv_chart_set_next_value(chart, serie1, Accel::get().y * 100);

        lv_label_set_text_fmt(label, "%u,%u,%u",
                              GET_UNUSED_STACK(waThdLVGL),
                              GET_UNUSED_STACK(waThdTick),
                              GET_UNUSED_STACK(waThdLabel));
        cpt += dir;
        if (cpt == 80) dir = -1;
        if (cpt == 0) dir = 1;
        chThdSleepMilliseconds(100);
    }
}
} // namespace

void initThreads(tprio_t prio)
{
    chThdCreateStatic(waThdLabel, sizeof(waThdLabel), prio, ThreadLabel, NULL);
    chThdCreateStatic(waThdLVGL, sizeof(waThdLVGL), prio + 20, ThreadLVGL, NULL);
    chThdCreateStatic(waThdTick, sizeof(waThdTick), prio + 30, ThreadTick, NULL);
}
} // namespace Display