 /*----------------------------------------------------------------------------
  Project :  STM32F1 I²C Busy Flag Erratum (Register)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __I2C_ERR_F1_H
#define __I2C_ERR_F1_H
#ifdef __cplusplus
extern "C" {
#endif

void I2C_ClearBusyFlagErratum(void); //STM32F1 시리즈 I²C Busy Flag 문제 해결

#ifdef __cplusplus
}
#endif
#endif /*__ I2C_ERR_F1_H */
