#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  板级初始化总入口
 */
void bsp_init(void);

/**
 * @brief 设置 RGB LED 单通道亮度
 * @param ch  TMR 通道选择 (TMR_SELECT_CHANNEL_1/2/3)
 * @param val 亮度 0~999 (0=灭, 999=最亮)
 */
void rgbled_set(uint16_t r, uint16_t g, uint16_t b);

/**
 * @brief 向 TPC112S1 DAC 写入 12 位码值
 * @param code DAC 码值 0~4095
 */
void dac_write(uint16_t code);

/**
 * @brief  启动 ADC DMA 采集
 * @details 配置 DMA 缓冲区并启动，ADC 由 TMR15 硬件触发，
 *          每次转换完成由 DMA 自动搬运到 adc_buf。
 */
void adc_start(void);

/**
 * @brief  停止 ADC DMA 采集
 */
void adc_stop(void);

/**
 * @brief  检查 ADC DMA 是否完成一轮采集
 * @return true  已采集满 ADC_BUF_LEN 个数据
 * @return false 采集中
 */
bool adc_is_finish(void);

/**
 * @brief  获取 ADC 采集平均值
 * @return 16x 过采样原始值的算术平均
 * @note   需先调用 adc_is_finish() 确认采集完成
 */
uint16_t adc_get_value(void);

/**
 * @brief 1 ms 中断
 */
void tick_handler(void);

/**
 * @brief 10kHz 触发中断
 */
void samplerate_10kHz_handler(void);

#endif
