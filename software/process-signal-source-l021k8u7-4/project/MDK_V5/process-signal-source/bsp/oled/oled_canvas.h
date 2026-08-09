#ifndef _OLED_CANVAS_H_
#define _OLED_CANVAS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 画点
 * @param x     X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_point(uint8_t x, uint8_t y, bool color);

/**
 * @brief 画线 (Bresenham)
 * @param x1    起点 X (0 ~ OLED_WIDTH - 1)
 * @param y1    起点 Y (0 ~ OLED_HEIGHT - 1)
 * @param x2    终点 X (0 ~ OLED_WIDTH - 1)
 * @param y2    终点 Y (0 ~ OLED_HEIGHT - 1)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool color);

/**
 * @brief 画矩形
 * @param x     左上角 X (0 ~ OLED_WIDTH - 1)
 * @param y     左上角 Y (0 ~ OLED_HEIGHT - 1)
 * @param w     宽度
 * @param h     高度
 * @param fill  true=填充, false=边框
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool fill, bool color);

/**
 * @brief 填充画布
 * @param color true=全白, false=全黑
 */
void canvas_fill(bool color);

/**
 * @brief 显示单个字符
 * @param x     X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param c     字符
 * @param size  字号 (FONTSIZE_0806 / FONTSIZE_1608)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_char(uint8_t x, uint8_t y, char c, uint8_t size, bool color);

/**
 * @brief 显示字符串
 * @param x     X 坐标 (0 ~ OLED_WIDTH - 1)
 * @param y     Y 坐标 (0 ~ OLED_HEIGHT - 1)
 * @param str   字符串
 * @param size  字号 (FONTSIZE_0806 / FONTSIZE_1608)
 * @param color true=点亮, false=熄灭
 */
void canvas_draw_string(uint8_t x, uint8_t y, const char *str, uint8_t size, bool color);

/**
 * @brief 全屏刷新 (帧缓冲 -> OLED)
 */
void canvas_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
