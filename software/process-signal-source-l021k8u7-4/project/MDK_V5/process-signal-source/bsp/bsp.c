#include "bsp.h"
#include "global_vars.h"
#include "key.h"
#include "wk_dma.h"
#include "at32l021_wk_config.h"

#define ADC_BUF_LEN 20
#define SINE_TABLE_SIZE 50

static uint16_t adc_buf[ADC_BUF_LEN];

static const float sine_table[SINE_TABLE_SIZE] = 
{
     0.000000f,  0.125333f,  0.248690f,  0.368125f,  0.481754f,
     0.587785f,  0.684547f,  0.770513f,  0.844328f,  0.904827f,
     0.951057f,  0.982287f,  0.998027f,  0.998027f,  0.982287f,
     0.951057f,  0.904827f,  0.844328f,  0.770513f,  0.684547f,
     0.587785f,  0.481754f,  0.368125f,  0.248690f,  0.125333f,
     0.000000f, -0.125333f, -0.248690f, -0.368125f, -0.481754f,
    -0.587785f, -0.684547f, -0.770513f, -0.844328f, -0.904827f,
    -0.951057f, -0.982287f, -0.998027f, -0.998027f, -0.982287f,
    -0.951057f, -0.904827f, -0.844328f, -0.770513f, -0.684547f,
    -0.587785f, -0.481754f, -0.368125f, -0.248690f, -0.125333f
};

/**
 * @brief  板级初始化总入口
 */
void bsp_init(void)
{
    key_add(KEY_FN_GPIO_PORT,  KEY_FN_PIN,  0);
    key_add(KEY_INC_GPIO_PORT, KEY_INC_PIN, 1);
    key_add(KEY_DEC_GPIO_PORT, KEY_DEC_PIN, 2);

    rgbled_set(0, 0, 0);

    adc_start();
}

/**
 * @brief 设置 RGB LED 单通道亮度
 * @param ch  TMR 通道选择 (TMR_SELECT_CHANNEL_1/2/3)
 * @param val 亮度 0~999 (0=灭, 999=最亮)
 */
void rgbled_set(uint16_t r, uint16_t g, uint16_t b)
{
    if (r > 999) r = 999;
    if (g > 999) g = 999;
    if (b > 999) b = 999;

    tmr_channel_value_set(TMR3, TMR_SELECT_CHANNEL_1, 999 - r);
    tmr_channel_value_set(TMR3, TMR_SELECT_CHANNEL_2, 999 - b);
    tmr_channel_value_set(TMR3, TMR_SELECT_CHANNEL_3, 999 - g);
}

/**
 * @brief 向 TPC112S1 DAC 写入 12 位码值
 * @param code DAC 码值 0~4095
 */
void dac_write(uint16_t code)
{
    if (code > 4095) code = 4095;

    gpio_bits_reset(DAC_SYNC_GPIO_PORT, DAC_SYNC_PIN);

    spi_i2s_data_transmit(SPI1, code & 0x0FFF);
    while (spi_i2s_flag_get(SPI1, SPI_I2S_TDBE_FLAG) == RESET);
    while (spi_i2s_flag_get(SPI1, SPI_I2S_RDBF_FLAG) == RESET);
    spi_i2s_data_receive(SPI1);

    gpio_bits_set(DAC_SYNC_GPIO_PORT, DAC_SYNC_PIN);
}

/**
 * @brief  启动 ADC DMA 采集
 * @details 先停 DMA，重新配置缓冲区地址和长度，清完成标志后使能。
 *          ADC 由 TMR15 CH1 硬件触发（~1kHz），每触发一次 DMA
 *          搬运一个 16-bit 过采样结果到 adc_buf。
 */
void adc_start(void)
{
    dma_channel_enable(DMA1_CHANNEL1, FALSE);
    wk_dma_channel_config(DMA1_CHANNEL1, (uint32_t)&ADC1->odt, (uint32_t)adc_buf, ADC_BUF_LEN);
    dma_flag_clear(DMA1_FDT1_FLAG);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);
}

/**
 * @brief  停止 ADC DMA 采集
 */
void adc_stop(void)
{
    dma_channel_enable(DMA1_CHANNEL1, FALSE);
}

/**
 * @brief  检查 ADC DMA 是否完成一轮采集
 * @return true  DMA 已搬运完 ADC_BUF_LEN 个数据
 * @return false 采集中
 */
bool adc_is_finish(void)
{
    return dma_flag_get(DMA1_FDT1_FLAG) != RESET;
}

/**
 * @brief  获取 ADC 采集平均值
 * @details 对 adc_buf 中所有采样值求算术平均。
 * @return 16x 过采样原始值平均 (0~65520)
 * @note   调用前应先通过 adc_is_finish() 确认采集完成。
 */
uint16_t adc_get_value(void)
{
    uint32_t sum = 0;
    for (int i = 0; i < ADC_BUF_LEN; i++)
    {
        sum += adc_buf[i];
    }
    return (uint16_t)(sum / ADC_BUF_LEN);
}

/**
 * @brief 1 ms 中断
 */
void tick_handler(void)
{
    g_vars.tick++;
    if ((g_vars.tick % 10) == 0)
    {
        key_scan();
    }

    if (adc_is_finish())
    {
        uint16_t raw = adc_get_value();

        g_vars.dac_volt = (float)raw / 65520.0f * AVDD_VOLT * 2.0f;
        adc_start();
    }
}

/**
 * @brief 50kHz DAC 波形更新中断
 */
void dac_update_handler(void)
{
    static uint8_t step = 0;

    if (g_vars.device_mode == MODE_DC)
    {
        g_vars.dac_vpp = 0; // 强制清零
        dac_write(g_vars.dac_offset);
    }
    else if (g_vars.device_mode == MODE_SQUARE)
    {
        if (step < 25)
        {
            dac_write(g_vars.dac_offset + g_vars.dac_vpp / 2);
        }
        else
        {
            dac_write(g_vars.dac_offset - g_vars.dac_vpp / 2);
        }
    }
    else if (g_vars.device_mode == MODE_SINE)
    {
        int16_t code = (int16_t)(g_vars.dac_offset + sine_table[step] * g_vars.dac_vpp / 2.0f);
        if (code < 0) code = 0;
        else if (code > 4095) code = 4095;
        dac_write((uint16_t)code);
    }

    step += 1;
    if (step >= 50)
    {
        step = 0;
    }
}
