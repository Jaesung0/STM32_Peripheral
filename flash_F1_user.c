/*----------------------------------------------------------------------------
  Project : STM32F1 플레시 마지막 1K 공간에 사용자데이터 저장
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License. 
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause

.ld 파일에 플레시 마지막 1K 공간을 사용자데이터 저장용으로 예약
 제품마다 SRAM 및 플레시 크기가 다르므로 주의
MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000,   LENGTH = 20K
  FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 63K
  USER     (rx)    : ORIGIN = 0x800FC00,   LENGTH = 1K
}
 ----------------------------------------------------------------------------*/
#include "main.h"
//#include "defines.h"
#include "flash_user.h"

                                 //64K                    128K                   254K
                                 //((uint32_t)0x0800FC00) ((uint32_t)0x0801FC00) ((uint32_t)0x0803FC00)
#define FLASH_USER_PAGE            ((uint32_t)0x0800FC00) 
#define FLASH_READ_WORD(ADDR)      (*(volatile uint32_t *)(ADDR))

//플래시 사용자영역 지우기
uint8_t FlashUser_Page_Erase(void)
{
  uint32_t PAGEError = 0;
  FLASH_EraseInitTypeDef EraseInitStruct;

  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_USER_PAGE;
  EraseInitStruct.NbPages = 1;

  HAL_FLASH_Unlock();
  HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError);
  HAL_FLASH_Lock();

  if(PAGEError == 0xFFFFFFFF)
    return SUCCESS;
  else
    return ERROR;
}

//FLASH 읽기, 8bit Data 반환 (Addr 범위 0~1023)
uint8_t FlashUser_Read_Byte(uint16_t Addr)
{
  uint32_t tmp;
  tmp = FLASH_USER_PAGE + (Addr & 0x03FF);

  //       ----Word 단위로 읽어서----         해당Byte가 있는곳으로 쉬프트 시킨후 추출하여 반환
  return (FLASH_READ_WORD(tmp&0xFFFFFFFC) >> (8 * (tmp%4)))&0xFF;
}

//FLASH HalfWord(2Byte) 읽기 (Addr 범위 0~1022)
uint16_t FlashUser_Read_HalfWord(uint16_t Addr)
{
  uint32_t tmp;
  uint16_t data;

  tmp = FLASH_USER_PAGE + (Addr & 0x03FE);
  data = *(volatile uint16_t *)tmp;

  return data;
}

//FLASH Word(4Byte) 읽기 (Addr 범위 0~1020)
uint32_t FlashUser_Read_Word(uint16_t Addr)
{
  uint32_t tmp;
  tmp = FLASH_USER_PAGE + (Addr & 0x03FC);

  return FLASH_READ_WORD(tmp);
}

//FLASH HalfWord(2Byte) 쓰기 (Addr 범위 0~1022)
uint8_t FlashUser_Program_HalfWord(uint16_t Addr, uint16_t Data)
{
  uint32_t tmp;
  tmp = FLASH_USER_PAGE + (Addr & 0x03FE);

  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, tmp, Data);
  HAL_FLASH_Lock();

  if(*(volatile uint16_t *)tmp ==  Data)
    return SUCCESS;
  else
    return ERROR;
}

//FLASH Word(4Byte) 쓰기 (Addr 범위 0~1020)
uint8_t FlashUser_Program_Word(uint16_t Addr, uint32_t Data)
{
  uint32_t tmp;
  tmp = FLASH_USER_PAGE + (Addr & 0x03FC);

  HAL_FLASH_Unlock();
  HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, tmp, Data);
  HAL_FLASH_Lock();

  if(*(volatile uint32_t *)tmp ==  Data)
    return SUCCESS;
  else
    return ERROR;
}


//FLASH 사용자 공간에 데이터 업데이트
//매개변수 1: Page 내에 빈 공간이 없을경우, 기록하지 않음 / Page 삭제를 수행 후 기록
//매개변수 2: 기록할 4Bytes 데이터, 0xFFFFFFF 는 기록하지 않고 종료하기 때문에 사용할수 없음

//  ADD+0  ADD+1  ADD+2  ADD+3
// +------+------+------+------+
// | 32bit little endian DATA  |
// +------+------+------+------+
uint8_t FlashUser_Update_Data(uint8_t Erase, uint32_t Data)
{
  uint32_t tmp32u;
  uint16_t addr;
  uint8_t  retval = SUCCESS;

  //비어있는 4Bytes 의 공간의 주소 찾기
  for(addr = 0; addr < 1024; addr+=4)
  {
    tmp32u = FlashUser_Read_Word(addr);

    if(tmp32u == 0xFFFFFFFF)
      break;
  }

  if(addr == 1024) //Page 내에 빈 공간이 없음
  {
    if(Erase) //Page 삭제
      retval = FlashUser_Page_Erase();
    else
      return ERROR;
  }

  //Page 삭제결과가 ERROR면 종료
  if(retval == ERROR)
     return ERROR;

  //Data가  0xFFFFFFFF 는 기록하지 않고 종료
  if(Data == 0xFFFFFFFF)
    return SUCCESS;

  return FlashUser_Program_Word(addr, Data);
}

//FLASH 사용자 공간에서 최종 데이터 가져오기
//반환값이 0xFFFFFFFF 이면 데이터가 없는것임
uint32_t FlashUser_Get_Data(void)
{
  uint32_t tmp32u;
  uint16_t addr;

  //비어있는 4Bytes 의 공간의 주소 찾기
  for(addr = 0; addr < 1024; addr+=4)
  {
    tmp32u = FlashUser_Read_Word(addr);

    if(tmp32u == 0xFFFFFFFF)
      break;
  }

  if(addr == 0)
    return 0xFFFFFFFF;
  else
    return FlashUser_Read_Word(addr-4);
}
