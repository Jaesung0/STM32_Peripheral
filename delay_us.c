/*----------------------------------------------------------------------------
  Project : STM32 MicroSecond delay
  TEXT Encoding : UTF-8

  SysTick를 이용한 MicroSecond 단위 delay
  코드의 출처 https://blog.naver.com/kiatwins/221034444044

  stm32Xxxx_it.c 의 SysTick_Handler 에 추가 할 내용
  //[MicroSecond delay]
    extern volatile uint32_t millis_cnt;
    millis_cnt++;
  ----------------------------------------------------------------------------*/
#include "delay_us.h"

volatile uint32_t millis_cnt;

void delay_us(uint32_t us)
{
  uint32_t temp, comp, micros, load, mil;
  uint8_t  flag = 0;

  load = SysTick->LOAD;
  micros = (millis_cnt&0x3FFFFF)*1000 + (load-SysTick->VAL)/((load+1)/1000);

  temp = micros;
  comp = temp + us;

  while(comp > temp)
  {
    mil = millis_cnt;

    if(((mil&0x3FFFFF)==0)&&(mil>0x3FFFFF)&&(flag==0))
      flag = 1;

    micros = (mil&0x3FFFFF)*1000 + (load-SysTick->VAL)/((load+1)/1000);

    if(flag)
      temp = micros + 0x400000UL * 1000;
    else
      temp = micros;
  }
}
