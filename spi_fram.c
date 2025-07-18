/*----------------------------------------------------------------------------
  Project : STM32 SPI F-RAM
  Author  : Jaesung Oh
            https://github.com/Jaesung0/STM32_Peripheral
  TEXT Encoding : UTF-8
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  This software is provided AS-IS.
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "spi_fram.h"

//User define include
//#include "User_define.h"

#ifndef SPIEEP_HANDLE
 #define SPIEEP_HANDLE     hspi1 //SPI1
#endif
#ifndef SPIEEP_CS_HIGH
 #define SPIEEP_CS_HIGH()  LL_GPIO_SetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif
#ifndef SPIEEP_CS_LOW
 #define SPIEEP_CS_LOW()   LL_GPIO_ResetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif

//Instruction set
#define SPIFRAM_WREN       0x06 //Set Write Enable Latch
#define SPIFRAM_WRDI       0x04 //Reset Write Enable Latch
#define SPIFRAM_RDSR       0x05 //Read Status register
#define SPIFRAM_WRSR       0x01 //Write Status register
#define SPIFRAM_READ       0x03 //Read from memory array
#define SPIFRAM_WRITE      0x02 //Write to memory array
#define SPIFRAM_READ_A8    0x0B //Read from memory array(M95040 A8 = 1)
#define SPIFRAM_WRITE_A8   0x0A //Write to memory array (M95040 A8 = 1)

extern SPI_HandleTypeDef  SPIFRAM_HANDLE;

static void SPIFRAM_WriteEnable(void)
{
  SPIFRAM_CS_LOW();
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, &(uint8_t){SPIFRAM_WREN,}, 1, 5);
  SPIFRAM_CS_HIGH();
}

static void SPIFRAM_WriteDisable(void)
{
  SPIFRAM_CS_LOW();
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, &(uint8_t){SPIFRAM_WRDI,}, 1, 5);
  SPIFRAM_CS_HIGH();
}

//상태 레지스터 읽기
uint8_t SPIFRAM_Read_StatusReg(void)
{
  uint8_t OPCode[2] = {SPIFRAM_RDSR, 0};
  uint8_t RxData[2] = {0,};

  SPIFRAM_CS_LOW();
  HAL_SPI_TransmitReceive(&SPIFRAM_HANDLE, OPCode, RxData, 2, 5);
  SPIFRAM_CS_HIGH();

  return RxData[1];
}

//상태 레지스터 쓰기
void SPIFRAM_Write_StatusReg(uint8_t velue)
{
  uint8_t OPCode[2] = {SPIFRAM_WRSR, 0};
  OPCode[1] = velue;

  //쓰기작업 활성화
  SPIFRAM_WriteEnable();

  SPIFRAM_CS_LOW();
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, OPCode, 2, 5);
  SPIFRAM_CS_HIGH();
}

//SPI F-RAM 읽기
//매개변수:
//  Type: 메모리 종류
//  Address: 시작주소
//  *pData:  데이터 포인터
//  DataSize: 데이터 크기
//  Timeout: 제한시간
//반환값:
//  0 정상완료
//  1 매개변수오류
//  2 읽기오류
//메모리 끝부분에 도달시: 롤오버 하지 않고 종료
uint8_t SPIFRAM_Read_Data(SPIFRAM_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize)
{
  uint8_t OPCode[4] = {SPIFRAM_READ, 0, };
  uint8_t OPsize;
  HAL_StatusTypeDef Status;

  if((pData == NULL) || (DataSize == 0))
    return 1;

  switch(Type)
  {               //용량      주소길이
    case FM25L04: //512bytes  1bit+1byte
      if(DataSize > (512-Address))
        DataSize = (512-Address);

      if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
        OPCode[0] = SPIFRAM_READ_A8;

      OPCode[1] = Address & 0xFF;
      OPsize = 2;
      break;

    case FM25L16: //2Kbytes  2bytes
      if(DataSize > (2048-Address))
        DataSize = (2048-Address);

      OPCode[1] = (Address >> 8) & 0x07;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25CL64: //8Kbytes  2bytes
      if(DataSize > (8192-Address))
        DataSize = (8192-Address);

      OPCode[1] = (Address >> 8) & 0x1F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V01: //16Kbytes  2bytes
      if(DataSize > (16384-Address))
        DataSize = (16384-Address);

      OPCode[1] = (Address >> 8) & 0x3F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V02: //32Kbytes  2bytes
      if(DataSize > (32768-Address))
        DataSize = (32768-Address);

      OPCode[1] = (Address >> 8) & 0x7F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V05: //64Kbytes  2bytes
      if(DataSize > (65536-Address))
        DataSize = (65536-Address);

      OPCode[1] = (Address >> 8) & 0xFF;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V10: //128Kbytes  3bytes
      if(DataSize > (131072-Address))
        DataSize = (131072-Address);

      OPCode[1] = (Address >> 16) & 0x01;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
     break;

    case FM25V20: //256Kbytes  3bytes
      if(DataSize > (262144-Address))
        DataSize = (262144-Address);

      OPCode[1] = (Address >> 16) & 0x03;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
      break;

    case FM25V40:  //512Kbytes  3bytes
      if(DataSize > (524288-Address))
        DataSize = (524288-Address);

      OPCode[1] = (Address >> 16) & 0x07;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
      break;

    default:
      return 1;
  }

  SPIFRAM_CS_LOW();
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, OPCode, OPsize, 5);
  Status = HAL_SPI_Receive(&SPIFRAM_HANDLE, pData, DataSize, 50);
  SPIFRAM_CS_HIGH();

  if(Status == HAL_OK)
    return 0;
  else
    return 2;
}

//SPI F-RAM 쓰기
//매개변수:
//  Type: 메모리 종류
//  Address: 시작주소
//  *pData:  데이터 포인터
//  DataSize: 데이터 크기
//  Timeout: 제한시간(Page 당 5ms 소요됨 고려)
//반환값:
//  0 정상완료
//  1 매개변수오류
//  2 쓰기오류
//메모리 끝부분에 도달시: 롤오버 하지 않고 종료
uint8_t SPIFRAM_Write_Data(SPIFRAM_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize)
{
  uint8_t OPCode[4] = {SPIFRAM_WRITE, 0, };
  uint8_t OPsize;
  HAL_StatusTypeDef Status;

  if((pData == NULL) || (DataSize == 0))
    return 1;

  switch(Type)
  {               //용량      주소길이
    case FM25L04: //512bytes  1bit+1byte
      if(DataSize > (512-Address))
        DataSize = (512-Address);

      if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
        OPCode[0] = SPIFRAM_WRITE_A8;

      OPCode[1] = Address & 0xFF;
      OPsize = 2;
      break;

    case FM25L16: //2Kbytes  2bytes
      if(DataSize > (2048-Address))
        DataSize = (2048-Address);

      OPCode[1] = (Address >> 8) & 0x07;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25CL64: //8Kbytes  2bytes
      if(DataSize > (8192-Address))
        DataSize = (8192-Address);

      OPCode[1] = (Address >> 8) & 0x1F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V01: //16Kbytes  2bytes
      if(DataSize > (16384-Address))
        DataSize = (16384-Address);

      OPCode[1] = (Address >> 8) & 0x3F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V02: //32Kbytes  2bytes
      if(DataSize > (32768-Address))
        DataSize = (32768-Address);

      OPCode[1] = (Address >> 8) & 0x7F;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V05: //64Kbytes  2bytes
      if(DataSize > (65536-Address))
        DataSize = (65536-Address);

      OPCode[1] = (Address >> 8) & 0xFF;
      OPCode[2] = Address & 0xFF;
      OPsize = 3;
      break;

    case FM25V10: //128Kbytes  3bytes
      if(DataSize > (131072-Address))
        DataSize = (131072-Address);

      OPCode[1] = (Address >> 16) & 0x01;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
      break;

    case FM25V20: //256Kbytes  3bytes
      if(DataSize > (262144-Address))
        DataSize = (262144-Address);

      OPCode[1] = (Address >> 16) & 0x03;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
      break;

    case FM25V40:  //512Kbytes  3bytes
      if(DataSize > (524288-Address))
        DataSize = (524288-Address);

      OPCode[1] = (Address >> 16) & 0x07;
      OPCode[2] = (Address >> 8) & 0xFF;
      OPCode[3] = Address & 0xFF;
      OPsize = 4;
      break;

    default:
      return 1;
  }

    //페이지 쓰기 진행
    SPIFRAM_WriteEnable();

    SPIFRAM_CS_LOW();
    HAL_SPI_Transmit(&SPIFRAM_HANDLE, OPCode, OPsize, 5);
    Status = HAL_SPI_Transmit(&SPIFRAM_HANDLE, pData, DataSize, 50);
    SPIFRAM_CS_HIGH();

    SPIFRAM_WriteDisable(); //FM25L04 Errata

  if(Status == HAL_OK)
    return 0;
  else
    return 2;
}
