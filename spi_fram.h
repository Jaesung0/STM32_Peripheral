/*----------------------------------------------------------------------------
  Project : STM32 SPI F-RAM
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __SPIFRAM_H
#define __SPIFRAM_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
              //   용량    주소길이
  FM25L04,    // 512bytes  1bit+1byte
  FM25L16,    //  2Kbytes  2bytes
  FM25CL64,   //  8Kbytes  2bytes
  FM25V01,    // 16Kbytes  2bytes
  FM25V02,    // 32Kbytes  2bytes
  FM25V05,    // 64Kbytes  2bytes
  FM25V10,    //128Kbytes  3bytes
  FM25V20,    //256Kbytes  3bytes
  FM25V40     //512Kbytes  3bytes
}SPIFRAM_Type;

uint8_t SPIFRAM_Read_StatusReg(void); //상태 레지스터 읽기
void SPIFRAM_Write_StatusReg(uint8_t velue); //상태 레지스터 쓰기
uint8_t SPIFRAM_Read_Data(SPIFRAM_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize); //SPI F-RAM 읽기
uint8_t SPIFRAM_Write_Data(SPIFRAM_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize);//SPI F-RAM 쓰기

#ifdef __cplusplus
}
#endif
#endif /*__ SPIFRAM_H */
