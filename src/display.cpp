#include "display.h"

#include <ILI9341_t3n.h>
#include <lvgl.h>

#include "global.h"
#include "miata.h"
//#include "body.h"

#define GET_UNUSED_STACK(wa) (chUnusedThreadStack(wa, sizeof(wa)))
#define GET_USED_STACK(wa) (sizeof(wa) - GET_UNUSED_STACK(wa))

namespace Display
{
namespace
{
THD_WORKING_AREA(waThdLVGL, 8 * 256);
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
    GlobalVars* pGV = (GlobalVars*)arg;

    pThdLabel = chThdGetSelfX();
    chEvtWaitAny(ALL_EVENTS);

    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_OFF);

    {
        // Show splash screen
        lv_obj_t* img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &miata);
        lv_obj_set_align(img, LV_ALIGN_TOP_LEFT);

        chThdSleepMilliseconds(500);

        pinMode(6, OUTPUT);
        digitalWrite(6, HIGH);

        chThdSleepMilliseconds(2000);

        lv_obj_del_async(img);
    }

    // global style
    lv_style_t style_main;
    lv_style_init(&style_main);
    lv_style_set_text_font(&style_main, &din1451_18);
    lv_style_set_bg_color(&style_main, lv_color_black());
    lv_style_set_text_color(&style_main, lv_color_white());
    lv_obj_add_style(lv_scr_act(), &style_main, LV_STATE_DEFAULT);

    // image to show when RPM is too high
    // lv_obj_t* img = lv_img_create(lv_scr_act());
    // lv_img_set_src(img, &body);
    // lv_obj_set_align(img, LV_ALIGN_BOTTOM_RIGHT);
    // lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);

    // info label (used to show stack usage)
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);

    // rpm meter
    lv_obj_t* meter = lv_meter_create(lv_scr_act());
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_set_pos(meter, 160, 20);
    lv_obj_set_size(meter, 220, 220);

    // rpm meter scale
    lv_meter_scale_t* scale = lv_meter_add_scale(meter);
    const int rot = 110;
    lv_meter_set_scale_range(meter, scale, 0, 80, (270 - rot) * 8 / 7, rot);
    lv_meter_set_scale_ticks(meter, scale, 8 * 4 + 1, 2, 10, lv_palette_main(LV_PALETTE_LIME));
    lv_meter_set_scale_major_ticks(meter, scale, 4, 4, 15, lv_color_white(), 15);

    // rpm meter needle
    lv_meter_indicator_t* indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_LIME), -10);

    // rpm label
    lv_obj_t* label_rpm = lv_label_create(lv_scr_act());
    lv_obj_align(label_rpm, LV_ALIGN_BOTTOM_RIGHT, -20, 0);


    // afr chart style
    lv_style_t style_chart;
    lv_style_init(&style_chart);
    lv_style_set_border_color(&style_chart, lv_color_white());
    lv_style_set_border_width(&style_chart, 1);

    // afr chart
    lv_obj_t* chart = lv_chart_create(lv_scr_act());
    lv_obj_remove_style(chart, NULL, LV_PART_MAIN);
    lv_obj_add_style(chart, &style_chart, LV_PART_MAIN);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 5, 0);
    lv_obj_set_size(chart, 145, 50);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -100, 100);
    lv_chart_set_point_count(chart, 160);

    // afr chart data
    lv_chart_series_t* ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_LIME), LV_CHART_AXIS_PRIMARY_Y);
    for (int i = lv_chart_get_point_count(chart); i > 0; i--)
    {
        lv_chart_set_next_value(chart, ser1, 0);
    }

    // accel chart style
    lv_style_t style_accel;
    lv_style_init(&style_accel);
    lv_style_set_border_color(&style_accel, lv_color_white());
    lv_style_set_border_width(&style_accel, 1);
    lv_style_set_bg_opa(&style_accel, 0);
    lv_style_set_radius(&style_accel, LV_RADIUS_CIRCLE);

    // accel chart
    const int accel_size = 72;
    lv_obj_t* accel_chart = lv_chart_create(lv_scr_act());
    lv_obj_remove_style(accel_chart, NULL, LV_PART_MAIN);
    lv_obj_add_style(accel_chart, &style_accel, LV_PART_MAIN);
    lv_obj_align(accel_chart, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    lv_obj_set_size(accel_chart, accel_size, accel_size);
    lv_obj_set_scrollbar_mode(accel_chart, LV_SCROLLBAR_MODE_OFF);
    lv_chart_set_type(accel_chart, LV_CHART_TYPE_SCATTER);
    lv_chart_set_range(accel_chart, LV_CHART_AXIS_PRIMARY_X, -100, 100);
    lv_chart_set_range(accel_chart, LV_CHART_AXIS_PRIMARY_Y, -100, 100);
    lv_chart_set_point_count(accel_chart, 3);

    // accel chart data
    lv_chart_series_t* accel_serie1 = lv_chart_add_series(accel_chart, lv_palette_main(LV_PALETTE_LIME), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_t* my_Cir = lv_obj_create(accel_chart);
    lv_obj_set_size(my_Cir, accel_size / 2, accel_size / 2);
    lv_obj_center(my_Cir);
    lv_obj_add_style(my_Cir, &style_accel, LV_PART_MAIN);
    lv_obj_t* my_hline = lv_obj_create(accel_chart);
    lv_obj_set_size(my_hline, 1, accel_size);
    lv_obj_center(my_hline);
    lv_obj_add_style(my_hline, &style_accel, LV_PART_MAIN);
    lv_obj_t* my_vline = lv_obj_create(accel_chart);
    lv_obj_set_size(my_vline, accel_size, 1);
    lv_obj_center(my_vline);
    lv_obj_add_style(my_vline, &style_accel, LV_PART_MAIN);

    // bool alert = false;

    for (;;)
    {
        uint16_t rpm = pGV->ms.rpm;

        lv_meter_set_indicator_value(meter, indic, rpm / 100);

        lv_label_set_text_fmt(label_rpm, "%u", rpm);

        lv_chart_set_next_value(chart, ser1, pGV->accel.y * 100);

        lv_chart_set_next_value2(accel_chart, accel_serie1, pGV->accel.x * 100, pGV->accel.y * 100);

        // bool new_alert =  rpm > 7000;
        // if (new_alert && !alert) lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
        // else if (!new_alert && alert) lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
        // alert = new_alert;

        lv_label_set_text_fmt(label, "%u,%u,%u,%u,%u,%u",
                              GET_UNUSED_STACK(waThdLVGL),
                              GET_UNUSED_STACK(waThdTick),
                              GET_UNUSED_STACK(waThdLabel),
                              pGV->accel.stack,
                              pGV->ms.stack,
                              pGV->gps.stack);

        chThdSleepMilliseconds(50);
    }
}
} // namespace

void initThreads(tprio_t prio, void* arg)
{
    chThdCreateStatic(waThdLabel, sizeof(waThdLabel), prio, ThreadLabel, arg);
    chThdCreateStatic(waThdLVGL, sizeof(waThdLVGL), prio + 20, ThreadLVGL, arg);
    chThdCreateStatic(waThdTick, sizeof(waThdTick), prio + 30, ThreadTick, arg);
}
} // namespace Display