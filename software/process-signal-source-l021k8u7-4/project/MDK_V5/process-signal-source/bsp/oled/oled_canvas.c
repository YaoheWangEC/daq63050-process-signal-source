#include "oled_canvas.h"
#include "oled.h"
#include "oled_io.h"
#include "oledfont.h"
#include <stdlib.h>

static uint8_t *frame_buf = &oled_buf[1];

/**
 * @brief 画点
 * @param x     X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_point(uint8_t x, uint8_t y, bool color)
{
    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
    {
        return;
    }

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;

    if (color)
    {
        frame_buf[x + page * OLED_WIDTH] |= (1 << bit);
    }
    else
    {
        frame_buf[x + page * OLED_WIDTH] &= ~(1 << bit);
    }
}

/**
 * @brief 画线 (Bresenham)
 * @param x1    起点 X (0 ~ OLED_WIDTH - 1)
 * @param y1    起点 Y (0 ~ OLED_HEIGHT - 1)
 * @param x2    终点 X (0 ~ OLED_WIDTH - 1)
 * @param y2    终点 Y (0 ~ OLED_HEIGHT - 1)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color)
{
    int dx  = abs((int)x2 - (int)x1);
    int dy  = -abs((int)y2 - (int)y1);
    int sx  = x1 < x2 ? 1 : -1;
    int sy  = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (1)
    {
        canvas_draw_point(x1, y1, color);

        if ((x1 == x2) && (y1 == y2))
        {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x1 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

/**
 * @brief 画矩形
 * @param x     左上角 X (0 ~ OLED_WIDTH - 1)
 * @param y     左上角 Y (0 ~ OLED_HEIGHT - 1)
 * @param w     宽度
 * @param h     高度
 * @param fill  true=填充, false=边框
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill, bool color)
{
    if (fill)
    {
        for (uint8_t r = y; r < y + h && r < OLED_HEIGHT; r++)
        {
            for (uint8_t c = x; c < x + w && c < OLED_WIDTH; c++)
            {
                canvas_draw_point(c, r, color);
            }
        }
    }
    else
    {
        canvas_draw_line(x, y, x + w - 1, y, color);
        canvas_draw_line(x, y, x, y + h - 1, color);
        canvas_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
        canvas_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    }
}

/**
 * @brief 填充画布
 * @param color true=全白, false=全黑
 */
void canvas_fill(bool color)
{
    uint8_t byte = color ? 0xFF : 0x00;
    uint16_t size = OLED_WIDTH * OLED_HEIGHT / 8;

    for (uint16_t i = 0; i < size; i++)
    {
        frame_buf[i] = byte;
    }
}

/**
 * @brief 显示单个字符
 * @param x     字符左上角 X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     字符左上角 Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param c     字符
 * @param size  字号 (FONTSIZE_0806 / FONTSIZE_1608)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_char(uint8_t x, uint8_t y, char c, uint8_t size, bool color)
{
    uint8_t idx = c - ' ';
    const uint8_t *font = NULL;
    uint8_t w, h;

    switch (size)
    {
    case FONTSIZE_0806:
        font = (const uint8_t *)ascii_0806[idx];
        w = 6;
        h = 8;
        break;
    case FONTSIZE_1608:
        font = (const uint8_t *)ascii_1608[idx];
        w = 8;
        h = 16;
        break;
    default:
        return;
    }

    for (uint8_t col = 0; col < w; col++)
    {
        for (uint8_t row = 0; row < h; row++)
        {
            uint8_t byte = font[col + (row / 8) * w];
            uint8_t bit  = (byte >> (row % 8)) & 0x01;

            if (bit)
            {
                canvas_draw_point(x + col, y + row, color);
            }
        }
    }
}

/**
 * @brief 显示字符串
 * @param x     字符串左上角 X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     字符串左上角 Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param str   字符串
 * @param size  字号 (FONTSIZE_0806 / FONTSIZE_1608)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t size, bool color)
{
    uint8_t char_w;

    switch (size)
    {
    case FONTSIZE_0806:
        char_w = 6;
        break;
    case FONTSIZE_1608:
        char_w = 8;
        break;
    default:
        return;
    }

    while (*str)
    {
        canvas_draw_char(x, y, *str, size, color);
        x += char_w;
        str++;
    }
}

/**
 * @brief 全屏刷新 (帧缓冲 -> OLED)
 */
void canvas_refresh(void)
{
    uint8_t pages = OLED_HEIGHT / 8;

    for (uint8_t page = 0; page < pages; page++)
    {
        oledio_set_position(0, page);
        oledio_write_data_buffer(&frame_buf[page * OLED_WIDTH], OLED_WIDTH);
    }
}
