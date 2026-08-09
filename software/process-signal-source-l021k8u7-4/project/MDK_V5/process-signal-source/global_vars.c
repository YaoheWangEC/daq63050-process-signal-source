#include "global_vars.h"
#include "bsp.h"

#include <string.h>

global_vars_t g_vars;

/**
 * @brief  全局变量初始化
 * @details 将 g_vars 全部归零，然后设置默认值。
 */
void global_vars_init(void)
{
    memset(&g_vars, 0x00, sizeof(g_vars));
    
    bsp_init();

    g_vars.uid[0] = *(volatile uint32_t *)0x1FFFF7E8;
    g_vars.uid[1] = *(volatile uint32_t *)0x1FFFF7EC;
    g_vars.uid[2] = *(volatile uint32_t *)0x1FFFF7F0;
    g_vars.tick = 0;

    g_vars.device_mode = MODE_DC;
    g_vars.dac_offset = 2495;
    g_vars.dac_vpp = 0;
    dac_write(g_vars.dac_offset);

}
