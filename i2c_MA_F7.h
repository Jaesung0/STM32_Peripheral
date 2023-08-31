/*----------------------------------------------------------------------------
  Project : STM32F7 I²C Master (Register)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __I2C_MA_F7_H
#define __I2C_MA_F7_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

#define I2C_Write_operation(I2C)       (I2C)->CR2 &= ~(0x00000400)
#define I2C_Read_operation(I2C)        (I2C)->CR2 |= 0x00000400
#define I2C_SetTransferSize(I2C, SIZE) (I2C)->CR2 = ((I2C)->CR2 & 0xFF00FFFF) | ((SIZE) << 16)
#define I2C_StartCondition(I2C)        (I2C)->CR2 |= 0x00002000
#define I2C_StopCondition(I2C)         (I2C)->CR2 |= 0x00004000
#define I2C_ClearFlag_STOP(I2C)        (I2C)->ICR = 0x00000020
#define I2C_EnableReloadMode(I2C)      (I2C)->CR2 |= 0x01000000
#define I2C_DisableReloadMode(I2C)     (I2C)->CR2 &= ~(0x01000000)
#define I2C_IsActiveFlag_TXE(I2C)      (I2C)->ISR & 0x00000001
#define I2C_IsActiveFlag_TXIS(I2C)     (I2C)->ISR & 0x00000002
#define I2C_IsActiveFlag_RXNE(I2C)     (I2C)->ISR & 0x00000004
#define I2C_IsActiveFlag_NACK(I2C)     (I2C)->ISR & 0x00000010
#define I2C_ClearFlag_NACK(I2C)        (I2C)->ICR = 0x00000010
#define I2C_IsActiveFlag_TC(I2C)       (I2C)->ISR & 0x00000040
#define I2C_IsActiveFlag_TCR(I2C)      (I2C)->ISR & 0x00000080
#define I2C_IsActiveFlag_BUSY(I2C)     (I2C)->ISR & 0x00008000
#define I2C_ClearFlag_ALL(I2C)         (I2C)->ICR = 0x00003F38
#define I2C_TransmitData(I2C, DATA)    (I2C)->TXDR = (DATA)
#define I2C_ReceiveData(I2C)           (I2C)->RXDR

uint8_t I2C_Stop(I2C_TypeDef *I2Cx, uint32_t Timeout); //I2C 종료
uint8_t I2C_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C Write
uint8_t I2C_Read( I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C Read
uint8_t I2C_TX_ADDW_Only(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Timeout); //I2C 디바이스주소+쓰기 후 종료
uint8_t I2C_Mem_Address_Set(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint32_t Timeout); //I2C memory 워드주소 쓰기 후 종료
uint8_t I2C_Mem_Current_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t Timeout); //I2C memory 현재주소 읽기
uint8_t I2C_Mem_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C memory 쓰기
uint8_t I2C_Mem_Read( I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C memory 읽기
uint8_t I2C_24xx_AckPoll(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Tickstart, uint32_t Timeout); //I2C Acknowledge Polling
uint8_t I2C_24xx_Write(I2C_TypeDef *I2Cx, uint16_t DevCapacity, uint8_t ChipAddress, uint32_t WordAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C EEPROM 쓰기
uint8_t I2C_24xx_Read( I2C_TypeDef *I2Cx, uint16_t DevCapacity, uint8_t ChipAddress, uint32_t WordAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //I2C EEPROM 쓰기
uint8_t I2C_TX_General_Call(I2C_TypeDef *I2Cx, uint8_t SecondByte, uint32_t Timeout); //I2C Transmit General Call

#ifdef __cplusplus
}
#endif
#endif /* __I2C_MA_F7_H */
