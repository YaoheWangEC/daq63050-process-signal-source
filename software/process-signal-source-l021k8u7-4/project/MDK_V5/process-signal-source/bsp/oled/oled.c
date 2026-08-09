#include "oled.h"
#include "oled_io.h"
#include "oledfont.h"
#include "wk_system.h"

static uint8_t *frame_buf = &oled_buf[1];

/**
 * @brief 初始化 OLED
 */
void oled_init(void)
{
    oledio_init();
    
    wk_delay_ms(200);

    oledio_write_cmd(0xAE); // display off

    oledio_write_cmd(0xD5); // set osc division
    oledio_write_cmd(0xF0);
    oledio_write_cmd(0xA8); // multiplex ratio
    oledio_write_cmd(0x27); // duty = 1/40
    oledio_write_cmd(0xD3); // set display offset
    oledio_write_cmd(0x00);
    oledio_write_cmd(0x40); // set display start line = 0

    oledio_write_cmd(0x8D); // charge pump enable
    oledio_write_cmd(0x14);

    oledio_write_cmd(0x20); // set page addressing mode
    oledio_write_cmd(0x02);

    oledio_write_cmd(0xA1); // segment remap (column 127 = SEG0)
    oledio_write_cmd(0xC8); // COM scan direction (remapped)

    oledio_write_cmd(0xDA); // set COM pins
    oledio_write_cmd(0x12);

    oledio_write_cmd(0xAD); // internal IREF setting
    oledio_write_cmd(0x30);

    oledio_write_cmd(0x81); // contrast control
    oledio_write_cmd(0xFF);

    oledio_write_cmd(0xD9); // set pre-charge period
    oledio_write_cmd(0x22);

    oledio_write_cmd(0xDB); // set vcomh
    oledio_write_cmd(0x20);

    oledio_write_cmd(0xA4); // entire display follows RAM
    oledio_write_cmd(0xA6); // normal display

    oled_fill(false);
    oledio_write_cmd(0xAF); // display ON
}

/**
 * @brief 设置显示亮度
 * @param val 0x01~0xFF (默认 0xFF)
 */
void oled_set_brightness(uint8_t val)
{
    oledio_write_cmd(0x81);
    oledio_write_cmd(val);
}

/**
 * @brief 全屏填充
 * @param color true=全白, false=全黑
 */
void oled_fill(bool color)
{
    uint8_t byte = color ? 0xFF : 0x00;
    uint16_t fb_size = OLED_WIDTH * OLED_HEIGHT / 8;

    for (uint16_t i = 0; i < fb_size; i++)
    {
        frame_buf[i] = byte;
    }

    for (uint8_t page = 0; page < OLED_HEIGHT / 8; page++)
    {
        oledio_set_position(0, page);
        oledio_write_data_buffer(&frame_buf[page * OLED_WIDTH], OLED_WIDTH);
    }
}

/**
 * @brief 显示单个字符
 * @param x      X 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y      Y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 * @param c      字符
 * @param size   字号 (FONTSIZE_xxx)
 * @param color  true=点亮, false=熄灭
 */
void oled_show_char(uint8_t x, uint8_t y, char c, uint8_t size, bool color)
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
        for (uint8_t page = 0; page < (h / 8); page++)
        {
            uint8_t d = font[col + page * w];
            if (!color) d = ~d;
            oledio_set_position(x + col, y + page);
            oledio_write_data(d);
        }
    }
}

/**
 * @brief 显示字符串
 * @param x      X 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y      Y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 * @param str    字符串
 * @param size   字号 (FONTSIZE_xxx)
 * @param color  true=点亮, false=熄灭
 */
void oled_show_string(uint8_t x, uint8_t y, const char *str, uint8_t size, bool color)
{
    uint8_t char_w;

    switch (size)
    {
    case FONTSIZE_0806: char_w = 6; break;
    case FONTSIZE_1608: char_w = 8; break;
    default: return;
    }

    while (*str)
    {
        oled_show_char(x, y, *str, size, color);
        x += char_w;
        str++;
    }
}
