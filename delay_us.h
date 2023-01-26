/*----------------------------------------------------------------------------
  Project : STM32 MicroSecond delay
  TEXT Encoding : UTF-8

  SysTick를 이용한 MicroSecond 단위 delay
  ----------------------------------------------------------------------------*/
#ifndef STM32_DELAY_US_H
#define STM32_DELAY_US_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif
#endif /*__ STM32_DELAY_US_H */
