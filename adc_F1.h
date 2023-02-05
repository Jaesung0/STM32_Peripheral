 /*----------------------------------------------------------------------------
  Project : STM32F1 ADC1
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __ADC_F1_H
#define __ADC_F1_H
#ifdef __cplusplus
extern "C" {
#endif

#define ADC1_TRIG_TIM      TIM3   //ACD 트리거 타이머
#define ADC1_CONV_FREQ     10     //1초당 변환수
#define ADC1_NBR_OF_CH     5      //채널합계, MX_ADC1_Init(void) 함수의 hadc1.Init.NbrOfConversion 과 동일하게 설정
#define ADC1_NBR_OF_OVS    5      //오버샘플수
#define ADC1_VREF_VOLT     3300   //VREF+ 전압
#define ADC1_VREFINT_RANK  1      //MCU내부 1.2V기준전압 채널의 랭크, 0으로 설정하면 ADC1_VREF_VOLT 값으로 계산
#define ADC1_USE_VOLT_DVI  1      //Voltage divider 을 감안한 전압값 활성화 (adc_XX.c 의 gADC1_VolDvi_Table 을 작성해야됨)

void ADC1_TIM_DMA_Enable(void);
uint16_t ADC1_GetMilliVolt(uint8_t number);

#ifdef __cplusplus
}
#endif
#endif /*__ ADC_F1_H */
