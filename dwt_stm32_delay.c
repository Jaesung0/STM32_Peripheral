/*----------------------------------------------------------------------------
  Project : STM32 MicroSecond delay
  TEXT Encoding : UTF-8

  DWT(Data Watchpoint and Trace)를 이용한 MicroSecond 단위 delay
  Cortex-M0 는 DWT가 지원되지 않습니다.

  코드의 1차 출처 http://stm32f4-discovery.net/2015/07/hal-library-3-delay-for-stm32fxxx/
  코드의 2차 출처 https://m.blog.naver.com/eziya76/221477231713
  코드의 3차 출처 https://mcutry.tistory.com/90?category=629416
  ----------------------------------------------------------------------------*/
#include "dwt_stm32_delay.h"

/**
 * @brief  Initializes DWT_Clock_Cycle_Count for DWT_Delay_us function
 * @return Error DWT counter
 *         1: clock cycle counter not started
 *         0: clock cycle counter works
 */
uint32_t DWT_Delay_Init(void)
{
  /* Disable TRC */
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
  /* Enable TRC */
  CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;

  /* Disable clock cycle counter */
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
  /* Enable  clock cycle counter */
  DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

  /* Reset the clock cycle counter value */
  DWT->CYCCNT = 0;

  /* 3 NO OPERATION instructions */
  //keil;  __nop;
  //IAR ;  __ASM volatile ("NOP");
  //GCC ;  asm("nop");  
  asm("nop"); asm("nop"); asm("nop");

  /* Check if clock cycle counter has started */
  if(DWT->CYCCNT)
       return 0; /*clock cycle counter started*/
  else
    return 1; /*clock cycle counter not started*/
}

/**
* @brief  This function provides a delay (in microseconds)
* @param  microseconds: delay in microseconds
*/
void DWT_Delay_us(volatile uint32_t microseconds)
{
  uint32_t clk_cycle_start = DWT->CYCCNT;

  /* Go to number of cycles for system */
  microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

  /* Delay till end */
  while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}

/* Use DWT_Delay_Init (); and DWT_Delay_us (microseconds) in the main */
