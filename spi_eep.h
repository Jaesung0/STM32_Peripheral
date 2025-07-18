/*----------------------------------------------------------------------------
  Project : STM32 SPI EEPROM
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __SPIEEP_H
#define __SPIEEP_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
              //   용량     페이지  주소길이
  AT25010,    // 128bytes   8bytes  1byte
  M95010,     // 128bytes  16bytes  1byte
  AT25020,    // 256bytes   8bytes  1byte
  M95020,     // 256bytes  16bytes  1byte
  AT25040,    // 512bytes   8bytes  1bit+1byte
  M95040,     // 512bytes  16bytes  1bit+1byte
  AT25080,    //  1Kbyte   16bytes  2bytes
  M95080,     //  1Kbyte   32bytes  2bytes
  AT25160,    //  2Kbytes  16bytes  2bytes
  M95160,     //  2Kbytes  32bytes  2bytes
  M95320,     //  4Kbytes  32bytes  2bytes
  M95640,     //  8Kbytes  32bytes  2bytes
  M95128,     // 16Kbytes  64bytes  2bytes
  M95256,     // 32Kbytes  64bytes  2bytes
  M95512,     // 64Kbytes 128bytes  2bytes
  M95M01,     //128Kbytes 256bytes  3bytes
  M95M02,     //256Kbytes 256bytes  3bytes
  _25SCM04,   //512Kbytes 256bytes  3bytes
  M95M04,     //512Kbytes 512bytes  3bytes
  
  FM25L04,    // 512bytes   None    1bit+1byte
  FM25L16,    //  2Kbytes   None    2bytes
  FM25CL64,   //  8Kbytes   None    2bytes
  FM25V01,    // 16Kbytes   None    2bytes
  FM25V02,    // 32Kbytes   None    2bytes
  FM25V05,    // 64Kbytes   None    2bytes
  FM25V10,    //128Kbytes   None    3bytes
  FM25V20,    //256Kbytes   None    3bytes
  FM25V40     //512Kbytes   None    3bytes
}SPIEEP_Type;

uint8_t SPIEEP_Read_StatusReg(void); //상태 레지스터 읽기
void SPIEEP_Write_StatusReg(uint8_t velue); //상태 레지스터 쓰기
uint8_t SPIEEP_Read_Data(SPIEEP_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize, uint32_t Timeout); //SPI EEPROM 읽기
uint8_t SPIEEP_Write_Data(SPIEEP_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize, uint32_t Timeout);//SPI EEPROM 쓰기

#ifdef __cplusplus
}
#endif
#endif /*__ SPIEEP_H */
