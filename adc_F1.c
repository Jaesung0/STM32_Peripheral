 /*----------------------------------------------------------------------------
  Project : STM32F1 ADC1
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License. 
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  ----------------------------------------------------------------------------*/
//#include <stdio.h>
#include "main.h"
#include "defines.h"
#include "adc_F1.h"

#if ADC1_USE_VOLT_DVI
 //Voltage divider에 장착된 저항 룩업테이블
 // 비율로 계산 하므로 단위는 무시
 // Voltage divide가 없는 RANK는 {  0,  0} 으로 적음
 const uint16_t gADC1_VolDvi_Table[ADC1_NBR_OF_CH][2] =
 {//{R1(상단),R2(하단)}
    {  0,  0}, //RANK1, Voltage divider 없음
    { 62, 30}, //RANK2, R1: 62k, R2: 30k
    { 30, 71}  //RANK3, R1: 30k, R2: 71k
               //RANK4...
 };
#endif

static uint16_t ADC1_DMA[ADC1_NBR_OF_CH];
static uint16_t gADC1_ave[ADC1_NBR_OF_CH], gADC1_mV[ADC1_NBR_OF_CH];

extern ADC_HandleTypeDef hadc1;

void ADC1_TIM_DMA_Enable(void)
{
  uint32_t tmp_u32;
  uint16_t Prescaler = 1;

  if(ADC1_TRIG_TIM == TIM1)
    tmp_u32 = HAL_RCC_GetPCLK2Freq(); //STM32F10x APB2 타이머 클럭
  else
    tmp_u32 = HAL_RCC_GetPCLK1Freq() * 2; //STM32F10x APB1 타이머 클럭

  if(ADC1_CONV_FREQ > 0)
    tmp_u32 = (float)tmp_u32  / (float)ADC1_CONV_FREQ;
  else
    tmp_u32 = tmp_u32 * 2; //AD_CONV_FREQ 가 0인경우 0.5Hz로 처리

   while(tmp_u32 > 65535)
  {
    Prescaler = Prescaler * 10;
    tmp_u32 = tmp_u32 / 10;
  }

  LL_TIM_SetPrescaler(ADC1_TRIG_TIM, (Prescaler-1));
  LL_TIM_SetAutoReload(ADC1_TRIG_TIM, (tmp_u32-1));

  //HAL_ADCEx_Calibration_Start(&hadc1); //ADC 자체 교정실행(ADC 내부커패시터)
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC1_DMA, ADC1_NBR_OF_CH);

  LL_TIM_SetCounter(ADC1_TRIG_TIM, 0);
  LL_TIM_EnableCounter(ADC1_TRIG_TIM);
  //LL_TIM_GenerateEvent_UPDATE(ADC1_TRIG_TIM);//수동으로 트리거 발생
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  static uint16_t ADC1_Temp[ADC1_NBR_OF_CH][ADC1_NBR_OF_OVS];
  static uint8_t idx1_1st = 1, idx1_loop = 0;
  uint32_t tmp_u32;

  if(hadc->Instance == ADC1)
  {
    for(uint8_t rank = 0; rank < ADC1_NBR_OF_CH; rank++ )
    {
      ADC1_Temp[rank][idx1_loop] = ADC1_DMA[rank];

      tmp_u32 = 0;

      for(uint8_t i = 0; i < idx1_1st; i++)
        tmp_u32 = tmp_u32 + ADC1_Temp[rank][i];

      gADC1_ave[rank] = tmp_u32 / idx1_1st; //이동평균값 저장
    }

    idx1_1st = (idx1_1st < (ADC1_NBR_OF_OVS)) ? idx1_1st+1 : idx1_1st;  //ADC1_NBR_OF_OVS 까지만 증가
    idx1_loop = (idx1_loop < (ADC1_NBR_OF_OVS - 1)) ? idx1_loop+1 : 0; //0부터 (ADC1_NBR_OF_OVS - 1) 순환

    //전압값으로 변환
    //V_ADD(mV) =  1200(mV) * 4095 / VREFINT_DATA
    //          = 4914000 / VREFINT_DATA

    //V_CHx(mV) = V_ADD(mV) / 4095 * ADC_DATAx
    //          = (4914000 / VREFINT_DATA) / 4095 * ADC_DATAx
    //          = 1200 * ADC_DATAx / VREFINT_DATA

    gADC1_mV[ADC1_VREFINT_RANK-1] = 4914000 / gADC1_ave[ADC1_VREFINT_RANK-1];

    for(uint8_t rank = 0; rank < ADC1_NBR_OF_CH; rank++ )
    {
      if(rank == ADC1_VREFINT_RANK-1)
        continue;

      #if ADC1_USE_VOLT_DVI
        if( gADC1_VolDvi_Table[rank][1] == 0 ) //이번 RANK 는 Voltage divider 가 없음
        {
          gADC1_mV[rank] = 1200UL * (uint32_t)gADC1_ave[rank] / (uint32_t)gADC1_ave[ADC1_VREFINT_RANK-1];
        }
        else //이번 RANK 는 Voltage divider 가 있음
        {
          //Vin = Vout / (R2 / (R2 + R1))
          //    = (1200 * ADC_DATAx / VREFINT_DATA) / (R2 / (R2 + R1))
          //    = (1200UL * ADC_DATAx * R2 + 1200UL * ADC_DATAx * R1) / ( R2 * VREFINT_DATA)
          gADC1_mV[rank] =   (1200UL * (uint32_t)gADC1_ave[rank] * (uint32_t)gADC1_VolDvi_Table[rank][1] + 1200UL * (uint32_t)gADC1_ave[rank] * (uint32_t)gADC1_VolDvi_Table[rank][0])
                           / ( (uint32_t)gADC1_ave[ADC1_VREFINT_RANK-1] * (uint32_t)gADC1_VolDvi_Table[rank][1]);
        }
      #else
        gADC1_mV[rank] = 1200UL * (uint32_t)gADC1_ave[rank] / (uint32_t)gADC1_ave[ADC1_VREFINT_RANK-1];
      #endif
    }

    //디버그
    //for(uint8_t ch = 0; ch < ADC1_NBR_OF_CH; ch++ )
    //  printf(" [%04u %04u %04umV]", ADC1_DMA[ch], gADC1_ave[ch], gADC1_mV[ch]);

    //printf("\r\n");
  }
}

uint16_t ADC1_GetMilliVolt(uint8_t number)
{
  if(number < ADC1_NBR_OF_CH)
    return gADC1_mV[number];
  else
    return 0;
}
