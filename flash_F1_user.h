/*----------------------------------------------------------------------------
  Project : STM32F1 플레시 마지막 1K 공간에 사용자데이터 저장
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8

.ld 파일에 플레시 마지막 1K 공간을 공장설정값 저장용으로 예약
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 20K
  FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 63K
  USER     (rx)    : ORIGIN = 0x800FC00,   LENGTH = 1K
}
 ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __FLASH_H
#define __FLASH_H
#ifdef __cplusplus
 extern "C" {
#endif

uint8_t  FlashUser_Page_Erase(void); //플래시 사용자영역 지우기
uint8_t  FlashUser_Read_Byte(uint16_t Addr); //FLASH Byte(1Byte) 읽기
uint16_t FlashUser_Read_HalfWord(uint16_t Addr); //FLASH HalfWord(2Byte) 읽기
uint32_t FlashUser_Read_Word(uint16_t Addr); //FLASH Word(4Byte) 읽기
uint8_t  FlashUser_Program_HalfWord(uint16_t Addr, uint16_t Data); //FLASH HalfWord(2Byte) 쓰기
uint8_t  FlashUser_Program_Word(uint16_t Addr, uint32_t Data); //FLASH Word(4Byte) 쓰기
uint8_t  FlashUser_Update_Data(uint8_t Erase, uint32_t Data); //FLASH 사용자 공간에 데이터 업데이트
uint32_t FlashUser_Get_Data(void); //FLASH 사용자 공간에서 최종 데이터 가져오기

#ifdef __cplusplus
}
#endif
#endif /*__ FLASH_H */
