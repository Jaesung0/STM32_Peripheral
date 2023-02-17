 /*----------------------------------------------------------------------------
  Project :  STM32F1 I²C Master (Register)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __I2C_F1_H
#define __I2C_F1_H
#ifdef __cplusplus
extern "C" {
#endif

#define I2C_Enable(I2Cx)                ((I2Cx)->CR1 |= 0x00000001)
#define I2C_Disable(I2Cx)               ((I2Cx)->CR1 &= ~(0x00000001))
#define I2C_IsEnabled(I2Cx)             ((I2Cx)->CR1 & 0x00000001)
#define I2C_StartCondition(I2Cx)        ((I2Cx)->CR1 |= 0x00000100)
#define I2C_StopCondition(I2Cx)         ((I2Cx)->CR1 |= 0x00000200)
#define I2C_AcknowledgeEnable(I2Cx)     ((I2Cx)->CR1 |= 0x00000400)
#define I2C_AcknowledgeDisable(I2Cx)    ((I2Cx)->CR1 &= ~(0x00000400))
#define I2C_EnableBitPOS(I2Cx)          ((I2Cx)->CR1 |= 0x00000800)
#define I2C_DisableBitPOS(I2Cx)         ((I2Cx)->CR1 &= ~(0x00000800))
#define I2C_EnableSoftwareReset(I2Cx)   ((I2Cx)->CR1 |= 0x00008000)
#define I2C_DisableSoftwareReset(I2Cx)  ((I2Cx)->CR1 &= ~(0x00008000))

#define I2C_IsActiveFlag_SB(I2Cx)       ((I2Cx)->SR1 & 0x00000001) //Start Bit in Master Mode
#define I2C_IsActiveFlag_ADDR(I2Cx)     ((I2Cx)->SR1 & 0x00000002) //Address Sent in Master Mode
#define I2C_IsActiveFlag_BTF(I2Cx)      ((I2Cx)->SR1 & 0x00000004) //Byte Transfer Finished
#define I2C_IsActiveFlag_RXNE(I2Cx)     ((I2Cx)->SR1 & 0x00000040) //Receive Data Register Not Empty
#define I2C_IsActiveFlag_TXE(I2Cx)      ((I2Cx)->SR1 & 0x00000080) //Transmit Data Register Empty
#define I2C_IsActiveFlag_BERR(I2Cx)     ((I2Cx)->SR1 & 0x00000100) //Bus Error
#define I2C_IsActiveFlag_ARLO(I2Cx)     ((I2Cx)->SR1 & 0x00000200) //Arbitration Lost
#define I2C_IsActiveFlag_AF(I2Cx)       ((I2Cx)->SR1 & 0x00000400) //Acknowledge Failure
#define I2C_IsActiveFlag_BUSY(I2Cx)     ((I2Cx)->SR2 & 0x00000002) //Bus Busy

#define I2C_ClearFlag_BERR(I2Cx)        ((I2Cx)->SR1 &= ~(0x00000100))
#define I2C_ClearFlag_ARLO(I2Cx)        ((I2Cx)->SR1 &= ~(0x00000200))
#define I2C_ClearFlag_AF(I2Cx)          ((I2Cx)->SR1 &= ~(0x00000400))

#define I2C_TransmitData(I2Cx, DATA)    ((I2Cx)->DR = (DATA))
#define I2C_ReceiveData(I2Cx)           ((I2Cx)->DR)

void I2C_ClearBusyFlagErratum(void); //STM32F1 시리즈 I2C 비지-플레그 문제 해결
uint8_t I2C_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout);
uint8_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout);
uint8_t I2C_TX_SLAW_Only(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Timeout);
uint8_t I2C_Mem_Address_Set(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint32_t Timeout); //I2C memory 워드주소 쓰기 후 종료
uint8_t I2C_Mem_Current_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t Timeout); //I2C memory 현재주소 읽기
uint8_t I2C_Mem_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C memory 쓰기
uint8_t I2C_Mem_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C memory 읽기

#ifdef __cplusplus
}
#endif
#endif /*__ I2C_F1_H */
