 /*----------------------------------------------------------------------------
  Project : STM32G0 TIMER (Low Layer)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __TIMER_H
#define __TIMER_H
#ifdef __cplusplus
extern "C" {
#endif

void BASE_TIM_Enable(TIM_TypeDef *TIMx, uint32_t frequency);

uint8_t PWM_TIM_Enable(TIM_TypeDef *TIMx, uint32_t Channel, uint32_t Polarity, uint16_t frequency, uint16_t resolution, uint16_t CompareValue);
uint8_t PWM_TIM_SetFrequency(TIM_TypeDef *TIMx, uint16_t frequency, uint16_t resolution);
void PWM_TIM_SetCompare(TIM_TypeDef *TIMx, uint32_t Channel, uint16_t CompareValue);

#ifdef __cplusplus
}
#endif
#endif /*__ TIMER_H */
