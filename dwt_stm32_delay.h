/*----------------------------------------------------------------------------
  Project : STM32 MicroSecond delay
  TEXT Encoding : UTF-8
  
  DWT(Data Watchpoint and Trace)를 이용한 MicroSecond 단위 delay
  Cortex-M0 는 DWT가 지원되지 않습니다.
  
  코드의 1차 출처 http://stm32f4-discovery.net/2015/07/hal-library-3-delay-for-stm32fxxx/
  코드의 2차 출처 https://m.blog.naver.com/eziya76/221477231713
  코드의 2차 출처 https://mcutry.tistory.com/90?category=629416
  GNU GPL v3 라이센스가 적용됩니다.
  ----------------------------------------------------------------------------*/
#ifndef __DWT_STM32_DELAY_H
#define __DWT_STM32_DELAY_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

/**
* @brief  Initializes DWT_Cycle_Count for DWT_Delay_us function
* @return Error DWT counter
*         1: DWT counter Error
*         0: DWT counter works
*/
uint32_t DWT_Delay_Init(void);


/**
* @brief  This function provides a delay (in microseconds)
* @param  microseconds: delay in microseconds
*/
void DWT_Delay_us(volatile uint32_t microseconds);

__STATIC_INLINE void DWT_Delay_us_INLINE(volatile uint32_t microseconds)
{
  uint32_t clk_cycle_start = DWT->CYCCNT;

  /* Go to number of cycles for system */
  microseconds *= (HAL_RCC_GetHCLKFreq() / 1000000);

  /* Delay till end */
  while ((DWT->CYCCNT - clk_cycle_start) < microseconds);
}

#ifdef __cplusplus
}
#endif
#endif /*__ DWT_STM32_DELAY_H */
