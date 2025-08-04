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

#ifndef SPIFRAM_HANDLE
 #define SPIFRAM_HANDLE     hspi1 //SPI1
#endif
#ifndef SPIFRAM_CS_HIGH
 #define SPIFRAM_CS_HIGH()  LL_GPIO_SetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif
#ifndef SPIFRAM_CS_LOW
 #define SPIFRAM_CS_LOW()   LL_GPIO_ResetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif

#define USE_STATIC_BUFFER  1

//Instruction set
#define SPIFRAM_WREN       0x06 //Set Write Enable Latch
#define SPIFRAM_WRDI       0x04 //Reset Write Enable Latch
#define SPIFRAM_RDSR       0x05 //Read Status register
#define SPIFRAM_WRSR       0x01 //Write Status register
#define SPIFRAM_READ       0x03 //Read from memory array
#define SPIFRAM_WRITE      0x02 //Write to memory array
#define SPIFRAM_READ_A8    0x0B //Read from memory array(M95040 A8 = 1)
#define SPIFRAM_WRITE_A8   0x0A //Write to memory array (M95040 A8 = 1)

#if USE_STATIC_BUFFER
 static uint8_t DataBuff[256];
#endif

extern SPI_HandleTypeDef  SPIFRAM_HANDLE;

static void SPIFRAM_WriteEnable(void)
{
  uint8_t CmdData = SPIFRAM_WREN;
  
  SPIFRAM_CS_LOW();
  //HAL_SPI_Transmit(&SPIFRAM_HANDLE, &(uint8_t){SPIFRAM_WREN,}, 1, 5);
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, &CmdData, 1, 5);
  SPIFRAM_CS_HIGH();
}

static void SPIFRAM_WriteDisable(void)
{
  uint8_t CmdData = SPIFRAM_WRDI;
  
  SPIFRAM_CS_LOW();
  //HAL_SPI_Transmit(&SPIFRAM_HANDLE, &(uint8_t){SPIFRAM_WRDI,}, 1, 5);
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, &CmdData, 1, 5);
  SPIFRAM_CS_HIGH();
}

//상태 레지스터 읽기
uint8_t SPIFRAM_Read_StatusReg(void)
{
  uint8_t CmdData[2] = {SPIFRAM_RDSR, 0};
  uint8_t RxData[2] = {0,};

  SPIFRAM_CS_LOW();
  HAL_SPI_TransmitReceive(&SPIFRAM_HANDLE, CmdData, RxData, 2, 5);
  SPIFRAM_CS_HIGH();

  return RxData[1];
}

//상태 레지스터 쓰기
void SPIFRAM_Write_StatusReg(uint8_t velue)
{
  uint8_t CmdData[2] = {SPIFRAM_WRSR, 0};
  CmdData[1] = velue;

  //쓰기작업 활성화
  SPIFRAM_WriteEnable();

  SPIFRAM_CS_LOW();
  HAL_SPI_Transmit(&SPIFRAM_HANDLE, CmdData, 2, 5);
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
  uint32_t ReadableSize;
  uint8_t CmdData[4] = {SPIFRAM_READ, 0, };
  uint8_t CmdSize;
  HAL_StatusTypeDef Status;

  if((pData == NULL) || (DataSize == 0))
    return 1;

  while(DataSize)
  {
    switch(Type)
    {               //용량      주소길이
      case FM25L04: //512bytes  1bit+1byte
        if(DataSize > (512-Address))
          DataSize = (512-Address);

        if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
          CmdData[0] = SPIFRAM_READ_A8;

        CmdData[1] = Address & 0xFF;
        CmdSize = 2;
        break;

      case FM25L16: //2Kbytes  2bytes
        if(DataSize > (2048-Address))
          DataSize = (2048-Address);

        CmdData[1] = (Address >> 8) & 0x07;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25CL64: //8Kbytes  2bytes
        if(DataSize > (8192-Address))
          DataSize = (8192-Address);

        CmdData[1] = (Address >> 8) & 0x1F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V01: //16Kbytes  2bytes
        if(DataSize > (16384-Address))
          DataSize = (16384-Address);

        CmdData[1] = (Address >> 8) & 0x3F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V02: //32Kbytes  2bytes
        if(DataSize > (32768-Address))
          DataSize = (32768-Address);

        CmdData[1] = (Address >> 8) & 0x7F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V05: //64Kbytes  2bytes
        if(DataSize > (65536-Address))
          DataSize = (65536-Address);

        CmdData[1] = (Address >> 8) & 0xFF;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V10: //128Kbytes  3bytes
        if(DataSize > (131072-Address))
          DataSize = (131072-Address);

        CmdData[1] = (Address >> 16) & 0x01;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
       break;

      case FM25V20: //256Kbytes  3bytes
        if(DataSize > (262144-Address))
          DataSize = (262144-Address);

        CmdData[1] = (Address >> 16) & 0x03;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
        break;

      case FM25V40:  //512Kbytes  3bytes
        if(DataSize > (524288-Address))
          DataSize = (524288-Address);

        CmdData[1] = (Address >> 16) & 0x07;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
        break;

      default:
        return 1;
    }

    if(Type == FM25L04)
      ReadableSize = 256 - (Address % 256);
    else if(DataSize > 256)
      ReadableSize = 256;

    if( DataSize < ReadableSize)
      ReadableSize = DataSize;

    SPIFRAM_CS_LOW();
    HAL_SPI_Transmit(&SPIFRAM_HANDLE, CmdData, CmdSize, 5);
    
   #if USE_STATIC_BUFFER
    Status = HAL_SPI_Receive(&SPIFRAM_HANDLE, DataBuff, ReadableSize, 50);
    
    for(uint32_t i=0; i<ReadableSize; i++)
      *(pData+i) = DataBuff[i];
   #else
    Status = HAL_SPI_Receive(&SPIFRAM_HANDLE, pData, ReadableSize, 50);
   #endif

    SPIFRAM_CS_HIGH();

    if(Status != HAL_OK)
      return 2;

    //DataSize가 0일 될때까지 반복
    Address += ReadableSize;
    pData += ReadableSize;
    DataSize -= ReadableSize;
  }

  return 0;
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
  uint32_t WritableSize;
  uint8_t CmdData[4] = {SPIFRAM_WRITE, 0, };
  uint8_t CmdSize;
  HAL_StatusTypeDef Status;

  if((pData == NULL) || (DataSize == 0))
    return 1;

  while(DataSize)
  {
    switch(Type)
    {               //용량      주소길이
      case FM25L04: //512bytes  1bit+1byte
        if(DataSize > (512-Address))
          DataSize = (512-Address);

        if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
          CmdData[0] = SPIFRAM_WRITE_A8;

        CmdData[1] = Address & 0xFF;
        CmdSize = 2;
        break;

      case FM25L16: //2Kbytes  2bytes
        if(DataSize > (2048-Address))
          DataSize = (2048-Address);

        CmdData[1] = (Address >> 8) & 0x07;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25CL64: //8Kbytes  2bytes
        if(DataSize > (8192-Address))
          DataSize = (8192-Address);

        CmdData[1] = (Address >> 8) & 0x1F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V01: //16Kbytes  2bytes
        if(DataSize > (16384-Address))
          DataSize = (16384-Address);

        CmdData[1] = (Address >> 8) & 0x3F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V02: //32Kbytes  2bytes
        if(DataSize > (32768-Address))
          DataSize = (32768-Address);

        CmdData[1] = (Address >> 8) & 0x7F;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V05: //64Kbytes  2bytes
        if(DataSize > (65536-Address))
          DataSize = (65536-Address);

        CmdData[1] = (Address >> 8) & 0xFF;
        CmdData[2] = Address & 0xFF;
        CmdSize = 3;
        break;

      case FM25V10: //128Kbytes  3bytes
        if(DataSize > (131072-Address))
          DataSize = (131072-Address);

        CmdData[1] = (Address >> 16) & 0x01;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
        break;

      case FM25V20: //256Kbytes  3bytes
        if(DataSize > (262144-Address))
          DataSize = (262144-Address);

        CmdData[1] = (Address >> 16) & 0x03;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
        break;

      case FM25V40:  //512Kbytes  3bytes
        if(DataSize > (524288-Address))
          DataSize = (524288-Address);

        CmdData[1] = (Address >> 16) & 0x07;
        CmdData[2] = (Address >> 8) & 0xFF;
        CmdData[3] = Address & 0xFF;
        CmdSize = 4;
        break;

      default:
        return 1;
    }

    if(Type == FM25L04)
      WritableSize = 256 - (Address % 256);
    else if(DataSize > 256)
      WritableSize = 256;

    if( DataSize < WritableSize)
      WritableSize = DataSize;

   #if USE_STATIC_BUFFER
    for(uint32_t i=0; i<WritableSize; i++)
      DataBuff[i] = *(pData+i);
   #endif

    SPIFRAM_WriteEnable();

    SPIFRAM_CS_LOW();
    HAL_SPI_Transmit(&SPIFRAM_HANDLE, CmdData, CmdSize, 5);

   #if USE_STATIC_BUFFER
    Status = HAL_SPI_Transmit(&SPIFRAM_HANDLE, DataBuff, WritableSize, 50);
   #else
    Status = HAL_SPI_Transmit(&SPIFRAM_HANDLE, pData, WritableSize, 50);
   #endif
    
    SPIFRAM_CS_HIGH();

    if(Status != HAL_OK)
    {
      if(Type == FM25L04)
        SPIFRAM_WriteDisable(); //FM25L04 Errata
      return 2;
    }

    //DataSize가 0일 될때까지 반복
    Address += WritableSize;
    pData += WritableSize;
    DataSize -= WritableSize;
  }

  if(Type == FM25L04)
    SPIFRAM_WriteDisable(); //FM25L04 Errata

   return 0;
}
