#include "user.h"
#include "global_vars.h"
#include "bsp.h"
#include "oled.h"
#include "oled_canvas.h"
#include "key.h"
#include "wk_system.h"
#include "command_parser.h"
#include "command_io.h"
#include "command_fifo.h"
#include "wk_wdt.h"
#include <stdio.h>

static int8_t select_item = 0; // 正在修改配置项，0直流偏置 1峰峰值
static bool configuring = false; // false切换选项 true修改值

static void display_handler(void);
static void key_handler(void);
static void perfomance_handler(void);

void setup(void)
{
    global_vars_init();

    
    wdt_counter_reload();
    oled_init();
    oled_set_brightness(0x10);
    oled_fill(true);
    oled_show_string(0, 0, "Hello world!", FONTSIZE_0806, false);
    oled_show_string(0, 2, "DAQ63050", FONTSIZE_0806, false);

    select_item = 0;
    configuring = false;

    command_fifo_init();
    command_io_init(&command_io_uart);
    command_parser_init();

    
    wdt_counter_reload();
    wk_delay_ms(100);
    wdt_counter_reload();
    wk_delay_ms(100);
    wdt_counter_reload();
    wk_delay_ms(100);
    wdt_counter_reload();
    wk_delay_ms(100);
    wdt_counter_reload();
    wk_delay_ms(100);
}

void loop(void)
{
    wdt_counter_reload();
    key_handler();
    command_parser_task();

    if (g_vars.tick % 100 == 0)
    {
        display_handler();
    }
    
    perfomance_handler();
}

static void display_handler(void)
{
    static char buf[12];

    canvas_fill(false);
    canvas_draw_rect(0, 0, OLED_WIDTH, OLED_HEIGHT, false, true);

    // 模式
    switch (g_vars.device_mode)
    {
    case MODE_DC:
        canvas_draw_string(3, 4, (const char *)"MODE DC", FONTSIZE_0806, true);
        break;
    case MODE_SQUARE:
        canvas_draw_string(3, 4, (const char *)"MODE SQUARE", FONTSIZE_0806, true);
        break;
    case MODE_SINE:
        canvas_draw_string(3, 4, (const char *)"MODE SINE", FONTSIZE_0806, true);
        break;
    }

    // 直流偏置
    if (select_item == 0)
    {
        if (configuring == false)
        {
            canvas_draw_line(1, 12, 1, 19, true);
        }
        else
        {
            canvas_draw_rect(1, 11, 70, 9, false, true);
        }
    }
    sprintf(buf, "DC   %4umV", (unsigned int)g_vars.dac_offset);
    canvas_draw_string(3, 12, buf, FONTSIZE_0806, true);

    // 峰峰值
    if (select_item == 1)
    {
        if (configuring == false)
        {
            canvas_draw_line(1, 20, 1, 27, true);
        }
        else
        {
            canvas_draw_rect(1, 19, 70, 9, false, true);
        }
    }
    sprintf(buf, "VPP  %4umV", (unsigned int)g_vars.dac_vpp);
    canvas_draw_string(3, 20, buf, FONTSIZE_0806, true);

    // DC 模式输出状态
    if (g_vars.device_mode == MODE_DC)
    {
        int32_t meas_mv = (int32_t)(g_vars.dac_volt * 1000.0f + 0.5f);
        int32_t dev_mv  = meas_mv - (int32_t)g_vars.dac_offset;

        if (dev_mv > DC_OVERLOAD_MV || dev_mv < -DC_OVERLOAD_MV)
        {
            canvas_draw_rect(2, 28, 50, 8, true, true);
            canvas_draw_string(3, 28, "OVERLOAD", FONTSIZE_0806, false);
        }
        else
        {
            canvas_draw_string(3, 28, "Normal", FONTSIZE_0806, true);
        }
    }

    canvas_refresh();
}

static void key_handler(void)
{
    static uint32_t key_lockout_until = 0; // 按键禁用到的tick值
    uint8_t key_id;
    key_event_t event;

    while (key_get_event(&key_id, &event))
    {
        if ((int32_t)(key_lockout_until - g_vars.tick) > 0)
            continue;

        if (key_id == 0) // FN
        {
            if (event == key_event_short)
            {
                configuring = !configuring; // 修改选项/切换选项
            }
            else if (event == key_event_long)
            {
                // 切换模式
                if (g_vars.device_mode == MODE_DC)
                {
                    switch_mode(MODE_SQUARE);
                }
                else if (g_vars.device_mode == MODE_SQUARE)
                {
                    switch_mode(MODE_SINE);
                }
                else
                {
                    switch_mode(MODE_DC);
                }

                key_lockout_until = g_vars.tick + 500;
            }
        }
        else if (key_id == 1) // INC
        {
            if (configuring == false) // 切换选项
            {
                select_item += 1;
                if (select_item >= 2)
                {
                    select_item = 0;
                }
            }
            else // 修改选项
            {
                int16_t step = (event == key_event_long) ? 10 : 1;

                if (select_item == 0) // 修改直流偏置
                {
                    set_dcoffset((int16_t)g_vars.dac_offset + step);
                }
                else // 修改峰峰值
                {
                    set_vpp((int16_t)g_vars.dac_vpp + step);
                }
            }
        }
        else if (key_id == 2) // DEC
        {
            if (configuring == false) // 切换选项
            {
                select_item -= 1;
                if (select_item < 0)
                {
                    select_item = 1;
                }
            }
            else // 修改选项
            {
                int16_t step = (event == key_event_long) ? 10 : 1;

                if (select_item == 0) // 修改直流偏置
                {
                    set_dcoffset((int16_t)g_vars.dac_offset - step);
                }
                else // 修改峰峰值
                {
                    set_vpp((int16_t)g_vars.dac_vpp - step);
                }
            }
        }
    }
}

static void perfomance_handler(void)
{
    static uint32_t delay_count = 0;
    static uint32_t last_report_tick = 0;
    const uint32_t total_delay_count = 426500; // 实测值

    // 步进
    wk_delay_us(10);
    delay_count += 1;

    // 每 5000ms 上报占用率
    if (g_vars.tick - last_report_tick >= 5000)
    {
        printf("CPU cost %d%% (%lu/%lu)\r\n",
               100 - (int32_t)(delay_count * 100 / total_delay_count),
               delay_count, total_delay_count);
        delay_count = 0;
        last_report_tick = g_vars.tick;
    }
}

bool switch_mode(device_mode_t next_mode)
{
    select_item = 0;
    configuring = false;

    switch (next_mode)
    {
    case MODE_DC:
        g_vars.device_mode = MODE_DC;
        g_vars.dac_offset = 2495;
        g_vars.dac_vpp = 0;
        return true;
    case MODE_SQUARE:
        g_vars.device_mode = MODE_SQUARE;
        g_vars.dac_offset = 1000;
        g_vars.dac_vpp = 1000;
        return true;
    case MODE_SINE:
        g_vars.device_mode = MODE_SINE;
        g_vars.dac_offset = 1000;
        g_vars.dac_vpp = 1000;
        return true;
    default:
        return false;
    }
}

bool set_dcoffset(int16_t next_value)
{
    if (next_value < 0 || next_value > 4095) return false;

    int32_t peak   = (int32_t)next_value + (int32_t)g_vars.dac_vpp / 2;
    int32_t valley = (int32_t)next_value - (int32_t)g_vars.dac_vpp / 2;

    if (valley < 0 || peak > 4095) return false;

    g_vars.dac_offset = (uint16_t)next_value;
    return true;
}

bool set_vpp(int16_t next_value)
{
    if (next_value < 0 || next_value > 4095) return false;

    int32_t peak   = (int32_t)g_vars.dac_offset + (int32_t)next_value / 2;
    int32_t valley = (int32_t)g_vars.dac_offset - (int32_t)next_value / 2;

    if (valley < 0 || peak > 4095) return false;

    g_vars.dac_vpp = (uint16_t)next_value;
    return true;
}
