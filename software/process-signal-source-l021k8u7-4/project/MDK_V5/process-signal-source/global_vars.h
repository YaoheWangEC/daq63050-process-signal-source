#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include <stdint.h>
#include <stdbool.h>

#define AVDD_VOLT 3.3f
#define VREF_VOLT 4.096f

typedef enum
{
    MODE_DC = 0,                /**< 直流输出模式 */
    MODE_SQUARE = 1,            /**< 1kHz 方波模式 */
    MODE_SINE = 2               /**< 1kHz 正弦波模式 */
} device_mode_t;

typedef struct 
{
    uint32_t uid[3];                /**< 器件 UID */
    uint32_t tick;                  /**< 1ms tick */

    device_mode_t device_mode;      /**< 输出模式 */
    uint16_t dac_offset;            /**< 输出直流偏置 mV */
    uint16_t dac_vpp;               /**< 输出峰峰值 mV */

    float dac_volt;                 /**< DAC 输出电压 */
} global_vars_t;

extern global_vars_t g_vars;

/**
 * @brief  全局变量初始化
 * @details 将 g_vars 全部归零，然后设置默认值。
 */
void global_vars_init(void);

#endif
