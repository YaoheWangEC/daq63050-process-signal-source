#ifndef _OLED_H_
#define _OLED_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 屏幕尺寸
#define OLED_WIDTH      72
#define OLED_HEIGHT     40

// 字号
#define FONTSIZE_0806   8
#define FONTSIZE_1608  16

/**
 * @brief 初始化 OLED
 */
void oled_init(void);

/**
 * @brief 设置显示亮度
 * @param val 0x01~0xFF (默认 0xFF)
 */
void oled_set_brightness(uint8_t val);

/**
 * @brief 全屏填充
 * @param color true=全白, false=全黑
 */
void oled_fill(bool color);

/**
 * @brief 显示单个字符
 * @param x      X 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y      Y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 * @param c      字符
 * @param size   字号 (FONTSIZE_xxx)
 * @param color  true=点亮, false=熄灭
 */
void oled_show_char(uint8_t x, uint8_t y, char c, uint8_t size, bool color);

/**
 * @brief 显示字符串
 * @param x      X 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y      Y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 * @param str    字符串
 * @param size   字号 (FONTSIZE_xxx)
 * @param color  true=点亮, false=熄灭
 */
void oled_show_string(uint8_t x, uint8_t y, const char *str, uint8_t size, bool color);

#ifdef __cplusplus
}
#endif

#endif // _OLED_H_
