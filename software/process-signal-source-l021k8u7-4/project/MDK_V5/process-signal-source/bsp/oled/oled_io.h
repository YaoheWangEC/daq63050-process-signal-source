#ifndef OLED_IO_H
#define OLED_IO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t oled_buf[];

/**
 * @brief 初始化 OLED IO 接口
 */
void oledio_init(void);

/**
 * @brief 写入一个命令字节
 * @param cmd 命令字
 */
void oledio_write_cmd(uint8_t cmd);

/**
 * @brief 写入一个数据字节
 * @param data 数据字
 */
void oledio_write_data(uint8_t data);

/**
 * @brief 批量写入数据
 * @param buf 数据起始指针
 * @param len 数据长度（字节）
 */
void oledio_write_data_buffer(const uint8_t *buf, uint16_t len);

/**
 * @brief 设置起始坐标（页寻址模式）
 * @param x 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 */
void oledio_set_position(uint8_t x, uint8_t y);

#ifdef __cplusplus
}
#endif

#endif // OLED_IO_H
