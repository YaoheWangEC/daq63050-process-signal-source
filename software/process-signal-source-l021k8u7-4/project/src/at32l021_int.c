/* add user code begin Header */
/**
  **************************************************************************
  * @file     at32l021_int.c
  * @brief    main interrupt service routines.
  **************************************************************************
  * Copyright (c) 2025, Artery Technology, All rights reserved.
  *
  * The software Board Support Package (BSP) that is made available to
  * download from Artery official website is the copyrighted work of Artery.
  * Artery authorizes customers to use, copy, and distribute the BSP
  * software and its related documentation for the purpose of design and
  * development in conjunction with Artery microcontrollers. Use of the
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */
/* add user code end Header */

/* includes ------------------------------------------------------------------*/
#include "at32l021_int.h"
/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "bsp.h"
#include "command_io.h"
/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */

/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */

/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

/* add user code end 0 */

/* external variables ---------------------------------------------------------*/
/* add user code begin external variables */

/* add user code end external variables */

/**
  * @brief  this function handles nmi exception.
  * @param  none
  * @retval none
  */
void NMI_Handler(void)
{
  /* add user code begin NonMaskableInt_IRQ 0 */

  /* add user code end NonMaskableInt_IRQ 0 */

  /* add user code begin NonMaskableInt_IRQ 1 */

  /* add user code end NonMaskableInt_IRQ 1 */
}

/**
  * @brief  this function handles hard fault exception.
  * @param  none
  * @retval none
  */
void HardFault_Handler(void)
{
  /* add user code begin HardFault_IRQ 0 */

  /* add user code end HardFault_IRQ 0 */
  /* go to infinite loop when hard fault exception occurs */
  while (1)
  {
    /* add user code begin W1_HardFault_IRQ 0 */

    /* add user code end W1_HardFault_IRQ 0 */
  }
}


/**
  * @brief  this function handles svcall exception.
  * @param  none
  * @retval none
  */
void SVC_Handler(void)
{
  /* add user code begin SVCall_IRQ 0 */

  /* add user code end SVCall_IRQ 0 */
  /* add user code begin SVCall_IRQ 1 */

  /* add user code end SVCall_IRQ 1 */
}

/**
  * @brief  this function handles pendsv_handler exception.
  * @param  none
  * @retval none
  */
void PendSV_Handler(void)
{
  /* add user code begin PendSV_IRQ 0 */

  /* add user code end PendSV_IRQ 0 */
  /* add user code begin PendSV_IRQ 1 */

  /* add user code end PendSV_IRQ 1 */
}


/**
  * @brief  this function handles systick handler.
  * @param  none
  * @retval none
  */
void SysTick_Handler(void)
{
  /* add user code begin SysTick_IRQ 0 */

  /* add user code end SysTick_IRQ 0 */

  /* add user code begin SysTick_IRQ 1 */

  /* add user code end SysTick_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 5 & 4 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel5_4_IRQHandler(void)
{
  /* add user code begin DMA1_Channel5_4_IRQ 0 */

  /* add user code end DMA1_Channel5_4_IRQ 0 */

  /* add user code begin DMA1_Channel5_4_IRQ 1 */
  /* USART4 TX DMA (DMA1_CHANNEL4) 传输完成: 清标志后交由命令框架善后 */
  if (dma_flag_get(DMA1_FDT4_FLAG) != RESET)
  {
    dma_flag_clear(DMA1_FDT4_FLAG);
    command_io_uart_tx_isr();
  }
  /* add user code end DMA1_Channel5_4_IRQ 1 */
}

/**
  * @brief  this function handles TMR6 handler.
  * @param  none
  * @retval none
  */
void TMR6_GLOBAL_IRQHandler(void)
{
  /* add user code begin TMR6_GLOBAL_IRQ 0 */

  /* add user code end TMR6_GLOBAL_IRQ 0 */

  /* overflow interrupt management */
  if(tmr_interrupt_flag_get(TMR6, TMR_OVF_FLAG) != RESET)
  {
    /* add user code begin TMR6_TMR_OVF_FLAG */
    /* clear flag */
    tmr_flag_clear(TMR6, TMR_OVF_FLAG);
    
    tick_handler();
    /* add user code end TMR6_TMR_OVF_FLAG */
  }

  /* add user code begin TMR6_GLOBAL_IRQ 1 */

  /* add user code end TMR6_GLOBAL_IRQ 1 */
}

/**
  * @brief  this function handles TMR14 handler.
  * @param  none
  * @retval none
  */
void TMR14_GLOBAL_IRQHandler(void)
{
  /* add user code begin TMR14_GLOBAL_IRQ 0 */

  /* add user code end TMR14_GLOBAL_IRQ 0 */

  /* overflow interrupt management */
  if(tmr_interrupt_flag_get(TMR14, TMR_OVF_FLAG) != RESET)
  {
    /* add user code begin TMR14_TMR_OVF_FLAG */
    /* clear flag */
    tmr_flag_clear(TMR14, TMR_OVF_FLAG);

    dac_update_handler();
    /* add user code end TMR14_TMR_OVF_FLAG */
  }

  /* add user code begin TMR14_GLOBAL_IRQ 1 */

  /* add user code end TMR14_GLOBAL_IRQ 1 */
}

/**
  * @brief  this function handles SPI1 handler.
  * @param  none
  * @retval none
  */
void SPI1_IRQHandler(void)
{
  /* add user code begin SPI1_IRQ 0 */

  /* add user code end SPI1_IRQ 0 */

  /* add user code begin SPI1_IRQ 1 */

  /* add user code end SPI1_IRQ 1 */
}

/**
  * @brief  this function handles USART4 & 3 handler.
  * @param  none
  * @retval none
  */
void USART4_3_IRQHandler(void)
{
  /* add user code begin USART4_3_IRQ 0 */

  /* add user code end USART4_3_IRQ 0 */

  if(usart_interrupt_flag_get(USART4, USART_IDLEF_FLAG) != RESET)
  {
    /* add user code begin USART4_USART_IDLEF_FLAG */
    /* clear flag */
    usart_flag_clear(USART4, USART_IDLEF_FLAG);

    command_io_uart_rx_isr();
    /* add user code end USART4_USART_IDLEF_FLAG */ 
  }

  /* add user code begin USART4_3_IRQ 1 */

  /* add user code end USART4_3_IRQ 1 */
}

/* add user code begin 1 */

/* add user code end 1 */
