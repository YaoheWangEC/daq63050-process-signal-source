#include "oled_io.h"
#include "oled.h"
#include "i2c_app.h"

#if (PLATFORM_DEVICE == DEVICE_AT32) && (PLATFORM_DRIVER == DRIVER_SPL)

#define OLED_I2C_HANDLE  &hi2c2
#define OLED_ADDR       (0x3C << 1)
#define OLED_CTRL_CMD   0x00
#define OLED_CTRL_DATA  0x40
#define OLED_X_OFFSET   28
#define OLED_TIMEOUT    1000000

uint8_t oled_buf[OLED_WIDTH * OLED_HEIGHT / 8 + 1];
static uint8_t tx_buf[2]; 

/**
 * @brief 初始化 OLED IO 接口
 */
void oledio_init(void)
{
    // do nothing
}

/**
 * @brief 写入一个命令字节
 * @param cmd 命令字
 */
void oledio_write_cmd(uint8_t cmd)
{
    tx_buf[0] = OLED_CTRL_CMD;
    tx_buf[1] = cmd;
    i2c_master_transmit(OLED_I2C_HANDLE, OLED_ADDR, tx_buf, 2, OLED_TIMEOUT);
}

/**
 * @brief 写入一个数据字节
 * @param data 数据字
 */
void oledio_write_data(uint8_t data)
{
    tx_buf[0] = OLED_CTRL_DATA;
    tx_buf[1] = data;
    i2c_master_transmit(OLED_I2C_HANDLE, OLED_ADDR, tx_buf, 2, OLED_TIMEOUT);
}

/**
 * @brief 批量写入数据
 * @param buf 数据缓冲区指针
 * @param len 数据长度（字节）
 */
void oledio_write_data_buffer(const uint8_t *buf, uint16_t len)
{
    if (len > sizeof(oled_buf) - 1) return;

    oled_buf[0] = OLED_CTRL_DATA;
    for (uint16_t i = 0; i < len; i++)
    {
        oled_buf[1 + i] = buf[i];
    }
    i2c_master_transmit(OLED_I2C_HANDLE, OLED_ADDR, oled_buf, len + 1, OLED_TIMEOUT);
}

/**
 * @brief 设置起始坐标（页寻址模式）
 * @param x 列坐标 (0 ~ OLED_WIDTH - 1)
 * @param y 页坐标 (0 ~ OLED_HEIGHT / 8 - 1)
 */
void oledio_set_position(uint8_t x, uint8_t y)
{
    x += OLED_X_OFFSET;

    oledio_write_cmd(0xB0 + y);
    oledio_write_cmd(((x & 0xF0) >> 4) | 0x10);
    oledio_write_cmd(x & 0x0F);
}

#endif
