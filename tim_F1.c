 /*----------------------------------------------------------------------------
  Project :  STM32F1 TIMER (Low Layer)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "defines.h"
#include "tim_F1.h"

void BASE_TIM_Enable(TIM_TypeDef *TIMx, uint32_t frequency)
{
  uint32_t tmp_u32;
  uint16_t Prescaler = 1;

  if(TIMx == TIM1)
    tmp_u32 = HAL_RCC_GetPCLK2Freq(); //STM32F10x APB2 타이머 클럭
  else
    tmp_u32 = HAL_RCC_GetPCLK1Freq() * 2; //STM32F10x APB1 타이머 클럭

  if(frequency > 0)
    tmp_u32 = (float)tmp_u32 / (float)frequency;
  else
    tmp_u32 = tmp_u32 * 2; //0인경우 0.5Hz로 처리

   while(tmp_u32 > 65535)
  {
    Prescaler = Prescaler * 10;
    tmp_u32 = tmp_u32 / 10;
  }

  LL_TIM_SetPrescaler(TIMx, (Prescaler-1));
  LL_TIM_SetAutoReload(TIMx, (tmp_u32-1));
  LL_TIM_SetCounter(TIMx, 0);

  LL_TIM_EnableIT_UPDATE(TIMx);
  //LL_TIM_SetUpdateSource(TIMx, LL_TIM_UPDATESOURCE_REGULAR);
  LL_TIM_EnableCounter(TIMx);
}

uint8_t PWM_TIM_Enable(TIM_TypeDef *TIMx, uint32_t Channel, uint32_t Polarity, float frequency, uint16_t resolution, uint16_t CompareValue)
{
  uint32_t tmp_u32;

  if(TIMx == TIM1)
    tmp_u32 = HAL_RCC_GetPCLK2Freq(); //STM32F10x APB2 타이머 클럭
  else
    tmp_u32 = HAL_RCC_GetPCLK1Freq() * 2; //STM32F10x APB1 타이머 클럭

  if( (frequency <= 0) || ( resolution <= 0))
    return 1;

  tmp_u32 = (float)tmp_u32 / frequency / (float)resolution;

  if(tmp_u32 > 65535)
    return 1;

  LL_TIM_SetPrescaler(TIMx, (tmp_u32-1));
  LL_TIM_SetAutoReload(TIMx, (resolution-1));
  LL_TIM_SetCounter(TIMx, 0);

  switch(Channel)
  {
    case LL_TIM_CHANNEL_CH1:
    case LL_TIM_CHANNEL_CH1N:
      LL_TIM_OC_SetCompareCH1(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH2:
    case LL_TIM_CHANNEL_CH2N:
      LL_TIM_OC_SetCompareCH2(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH3:
    case LL_TIM_CHANNEL_CH3N:
      LL_TIM_OC_SetCompareCH3(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH4:
      LL_TIM_OC_SetCompareCH4(TIMx, CompareValue);
      break;

    default:
      return 1;
      break;
  }

  LL_TIM_OC_SetPolarity(TIMx, Channel, Polarity);
  LL_TIM_CC_EnableChannel(TIMx, Channel);

  //TIM1 은 TIMx->BDTR 의 MOE을 1로 해야 PWM출력이됨
  if(TIMx == TIM1)
    LL_TIM_EnableAllOutputs(TIMx);

  LL_TIM_EnableCounter(TIMx);

  return 0;
}

uint8_t PWM_TIM_SetFrequency(TIM_TypeDef *TIMx, float frequency, uint16_t resolution)
{
  uint32_t tmp_u32;

  if(TIMx == TIM1)
    tmp_u32 = HAL_RCC_GetPCLK2Freq(); //STM32F10x APB2 타이머 클럭
  else
    tmp_u32 = HAL_RCC_GetPCLK1Freq() * 2; //STM32F10x APB1 타이머 클럭

  if( (frequency <= 0) || ( resolution <= 0))
    return 1;

  tmp_u32 = (float)tmp_u32 / frequency / (float)resolution;

  if(tmp_u32 > 65535)
    return 1;

  LL_TIM_SetPrescaler(TIMx, (tmp_u32-1));
  LL_TIM_SetAutoReload(TIMx, (resolution-1));
  LL_TIM_SetCounter(TIMx, 0);

  return 0;
}

void PWM_TIM_SetCompare(TIM_TypeDef *TIMx, uint32_t Channel, uint16_t CompareValue)
{
  switch(Channel)
  {
    case LL_TIM_CHANNEL_CH1:
    case LL_TIM_CHANNEL_CH1N:
      LL_TIM_OC_SetCompareCH1(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH2:
    case LL_TIM_CHANNEL_CH2N:
      LL_TIM_OC_SetCompareCH2(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH3:
    case LL_TIM_CHANNEL_CH3N:
      LL_TIM_OC_SetCompareCH3(TIMx, CompareValue);
      break;

    case LL_TIM_CHANNEL_CH4:
      LL_TIM_OC_SetCompareCH4(TIMx, CompareValue);
      break;

    default:
      break;
  }
}
