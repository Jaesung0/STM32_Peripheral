/*----------------------------------------------------------------------------
  Project : STM32 SPI EEPROM
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
#include "spi_eep.h"

//User define include
//#include "User_define.h"

//쓰기 완료대기 함수에서 사용 할 Delay 명령
#include "delay_us.h"
#define SPIEEP_DELAY       delay_us(500)

#ifndef SPIEEP_HANDLE
 #define SPIEEP_HANDLE     hspi1 //SPI1
#endif
#ifndef SPIEEP_CS_HIGH
 #define SPIEEP_CS_HIGH()  LL_GPIO_SetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif
#ifndef SPIEEP_CS_LOW
 #define SPIEEP_CS_LOW()   LL_GPIO_ResetOutputPin(SPI1_nCS_GPIO_Port, SPI1_nCS_Pin)
#endif

//SPI EEP Instruction set
#define SPIEEP_WREN       0x06 //Set Write Enable Latch
#define SPIEEP_WRDI       0x04 //Reset Write Enable Latch
#define SPIEEP_RDSR       0x05 //Read Status register
#define SPIEEP_WRSR       0x01 //Write Status register
#define SPIEEP_READ       0x03 //Read from memory array
#define SPIEEP_WRITE      0x02 //Write to memory array
#define SPIEEP_READ_A8    0x0B //Read from memory array(M95040 A8 = 1)
#define SPIEEP_WRITE_A8   0x0A //Write to memory array (M95040 A8 = 1)

extern SPI_HandleTypeDef  SPIEEP_HANDLE;

static void SPIEEP_WriteEnable(void)
{
  SPIEEP_CS_LOW();
  HAL_SPI_Transmit(&SPIEEP_HANDLE, &(uint8_t){SPIEEP_WREN,}, 1, 5);
  SPIEEP_CS_HIGH();
}

/*
static void SPIEEP_WriteDisable(void)
{
  SPIEEP_CS_LOW();
  HAL_SPI_Transmit(&SPIEEP_HANDLE, &(uint8_t){SPIEEP_WRDI,}, 1, 5);
  SPIEEP_CS_HIGH();
}
*/

//쓰기가 완료될때까지 대기
static uint8_t SPIEEP_WaitStandbyState(uint32_t Timeout)
{
  uint32_t Tickstart = uwTick;
  uint8_t RxData = 0;

  if(Timeout == 0)
    Timeout = 1;

  SPIEEP_CS_LOW();
  HAL_SPI_Transmit(&SPIEEP_HANDLE, &(uint8_t){SPIEEP_RDSR,}, 1, 5);

  while(1)
  {
    if((uwTick - Tickstart) > Timeout)
    {
      SPIEEP_CS_HIGH();
      return 1;
    }

    HAL_SPI_Receive(&SPIEEP_HANDLE, &RxData, 1, 5);

    if( (RxData & 0x01) == 0 )
      break;

    SPIEEP_DELAY;
  }

  SPIEEP_CS_HIGH();

  return 0;
}

//상태 레지스터 읽기
uint8_t SPIEEP_Read_StatusReg(void)
{
  uint8_t TxData[2] = {SPIEEP_RDSR, 0};
  uint8_t RxData[2] = {0,};

  SPIEEP_CS_LOW();
  HAL_SPI_TransmitReceive(&SPIEEP_HANDLE, TxData, RxData, 2, 5);
  SPIEEP_CS_HIGH();

  return RxData[1];
}

//상태 레지스터 쓰기
void SPIEEP_Write_StatusReg(uint8_t velue)
{
  uint8_t TxData[2] = {SPIEEP_WRSR, 0};
  TxData[1] = velue;

  //이전에 진행중인 쓰기작업 완료대기
  SPIEEP_WaitStandbyState(5);

  //쓰기작업 활성화
  SPIEEP_WriteEnable();

  SPIEEP_CS_LOW();
  HAL_SPI_Transmit(&SPIEEP_HANDLE, TxData, 2, 5);
  SPIEEP_CS_HIGH();

  //쓰기작업 완료 대기
  //SPIEEP_WaitStandbyState(5);
}

//SPI EEPROM 읽기
//매개변수:
//  Type: 메모리 종류
//  Address: 시작주소
//  *pData:  데이터 포인터
//  DataSize: 데이터 크기
//  Timeout: 제한시간
//반환값:
//  0 정상완료
//  1 매개변수오류
//  2 읽기오류(시간초과)
//메모리 끝부분에 도달시: 롤오버 하지 않고 종료
uint8_t SPIEEP_Read_Data(SPIEEP_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint8_t TxData[4] = {SPIEEP_READ, 0, };
  uint8_t TxSize;
  HAL_StatusTypeDef Status;

  if((pData == NULL) || (DataSize == 0))
    return 1;

  if(Timeout == 0)
    Timeout = 1;

  switch(Type)
  {               //용량,페이지 크기, 주소길이
    case AT25010: //128bytes  8bytes 1byte
    case M95010:  //128bytes 16bytes 1byte
      if(DataSize > (128-Address))
        DataSize = (128-Address);

      TxData[1] = Address & 0x7F;
      TxSize = 2;
      break;

    case AT25020: //256bytes  8bytes 1byte
    case M95020:  //256bytes 16bytes 1byte
      if(DataSize > (256-Address))
        DataSize = (256-Address);

      TxData[1] = Address & 0xFF;
      TxSize = 2;
      break;

    case AT25040: //512bytes  8bytes 1bit+1byte
    case M95040:  //512bytes 16bytes 1bit+1byte
    case FM25L04: //512bytes    None 1bit+1byte
      if(DataSize > (512-Address))
        DataSize = (512-Address);

      if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
        TxData[0] = SPIEEP_READ_A8;

      TxData[1] = Address & 0xFF;
      TxSize = 2;
      break;

    case AT25080: //1Kbyte 16bytes 2bytes
    case M95080:  //1Kbyte 32bytes 2bytes
      if(DataSize > (1024-Address))
        DataSize = (1024-Address);

      TxData[1] = (Address >> 8) & 0x03;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case AT25160: //2Kbytes 16bytes 2bytes
    case M95160:  //2Kbytes 32bytes 2bytes
    case FM25L16: //2Kbytes    None 2bytes
      if(DataSize > (2048-Address))
        DataSize = (2048-Address);

      TxData[1] = (Address >> 8) & 0x07;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95320:  //4Kbytes 32bytes 2bytes
      if(DataSize > (4096-Address))
        DataSize = (4096-Address);

      TxData[1] = (Address >> 8) & 0x0F;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95640:   //8Kbytes 32bytes 2bytes
    case FM25CL64: //8Kbytes    None 2bytes
      if(DataSize > (8192-Address))
        DataSize = (8192-Address);

      TxData[1] = (Address >> 8) & 0x1F;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95128:  //16Kbytes 64bytes 2bytes
    case FM25V01: //16Kbytes    None 2bytes
      if(DataSize > (16384-Address))
        DataSize = (16384-Address);

      TxData[1] = (Address >> 8) & 0x3F;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95256:  //32Kbytes 64bytes 2bytes
    case FM25V02: //32Kbytes    None 2bytes
      if(DataSize > (32768-Address))
        DataSize = (32768-Address);

      TxData[1] = (Address >> 8) & 0x7F;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95512:  //64Kbytes 128bytes 2bytes
    case FM25V05: //64Kbytes     None 2bytes
      if(DataSize > (65536-Address))
        DataSize = (65536-Address);

      TxData[1] = (Address >> 8) & 0xFF;
      TxData[2] = Address & 0xFF;
      TxSize = 3;
      break;

    case M95M01:  //128Kbytes 256bytes 3bytes
    case FM25V10: //128Kbytes     None 3bytes
      if(DataSize > (131072-Address))
        DataSize = (131072-Address);

      TxData[1] = (Address >> 16) & 0x01;
      TxData[2] = (Address >> 8) & 0xFF;
      TxData[3] = Address & 0xFF;
      TxSize = 4;
     break;

    case M95M02:  //256Kbytes 256bytes 3bytes
    case FM25V20: //256Kbytes     None 3bytes
      if(DataSize > (262144-Address))
        DataSize = (262144-Address);

      TxData[1] = (Address >> 16) & 0x03;
      TxData[2] = (Address >> 8) & 0xFF;
      TxData[3] = Address & 0xFF;
      TxSize = 4;
      break;

    case _25SCM04: //512Kbytes 256bytes 3bytes
    case M95M04:   //512Kbytes 512bytes 3bytes
    case FM25V40:  //512Kbytes     None 3bytes
      if(DataSize > (524288-Address))
        DataSize = (524288-Address);

      TxData[1] = (Address >> 16) & 0x07;
      TxData[2] = (Address >> 8) & 0xFF;
      TxData[3] = Address & 0xFF;
      TxSize = 4;
      break;

    default:
      return 1;
  }

  SPIEEP_CS_LOW();
  HAL_SPI_Transmit(&SPIEEP_HANDLE, TxData, TxSize, 5);
  Status = HAL_SPI_Receive(&SPIEEP_HANDLE, pData, DataSize, Timeout);
  SPIEEP_CS_HIGH();

  if(Status == HAL_OK)
    return 0;
  else
    return 2;
}

//SPI EEPROM 쓰기
//매개변수:
//  Type: 메모리 종류
//  Address: 시작주소
//  *pData:  데이터 포인터
//  DataSize: 데이터 크기
//  Timeout: 제한시간(Page 당 5ms 소요됨 고려)
//반환값:
//  0 정상완료
//  1 매개변수오류
//  2 시간초과
//Page 경계가 넘어가면 다음Page에 계속 기록, 메모리 끝부분에 도달시 중단
uint8_t SPIEEP_Write_Data(SPIEEP_Type Type, uint32_t Address, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick;
  uint32_t PageSize, WritableSize;
  uint8_t TxData[4] = {SPIEEP_WRITE, 0, };
  uint8_t TxSize;

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 1;

  //이전에 진행중인 쓰기작업 완료대기
  SPIEEP_WaitStandbyState(5);

  while(DataSize)
  {
    switch(Type)
    {               //용량,페이지 크기, 주소길이
      case AT25010: //128bytes  8bytes 1byte
      case M95010:  //128bytes 16bytes 1byte
        if(DataSize > (128-Address))
          DataSize = (128-Address);

        TxData[1] = Address & 0x7F;
        TxSize = 2;

        if(Type == M95010)
          PageSize = 16;
        else
          PageSize = 8;

        break;

      case AT25020: //256bytes  8bytes 1byte
      case M95020:  //256bytes 16bytes 1byte
        if(DataSize > (256-Address))
          DataSize = (256-Address);

        TxData[1] = Address & 0xFF;
        TxSize = 2;

        if(Type == M95020)
          PageSize = 16;
        else
          PageSize = 8;

        break;

      case AT25040: //512bytes  8bytes 1bit+1byte
      case M95040:  //512bytes 16bytes 1bit+1byte
      case FM25L04: //512bytes    None 1bit+1byte
        if(DataSize > (512-Address))
          DataSize = (512-Address);

        if(Address & 0x100) //주소의 A8부분은 Instruction set에 설정
          TxData[0] = SPIEEP_WRITE_A8;

        TxData[1] = Address & 0xFF;
        TxSize = 2;

        if(Type == FM25L04)
          PageSize = 512;
        else if(Type == M95040)
          PageSize = 16;
        else
          PageSize = 8;

        break;

      case AT25080: //1Kbyte 16bytes 2bytes
      case M95080:  //1Kbyte 32bytes 2bytes
        if(DataSize > (1024-Address))
          DataSize = (1024-Address);

        TxData[1] = (Address >> 8) & 0x03;
        TxData[2] = Address & 0xFF;
        TxSize = 3;

        if(Type == M95080)
          PageSize = 32;
        else
          PageSize = 16;

        break;

      case AT25160: //2Kbytes 16bytes 2bytes
      case M95160:  //2Kbytes 32bytes 2bytes
      case FM25L16: //2Kbytes    None 2bytes
        if(DataSize > (2048-Address))
          DataSize = (2048-Address);

        TxData[1] = (Address >> 8) & 0x07;
        TxData[2] = Address & 0xFF;
        TxSize = 3;

        if(Type == FM25L16)
          PageSize = 2048;
        else if(Type == M95160)
          PageSize = 32;
        else
          PageSize = 16;

        break;

      case M95320:  //4Kbytes 32bytes 2bytes
        if(DataSize > (4096-Address))
          DataSize = (4096-Address);

        TxData[1] = (Address >> 8) & 0x0F;
        TxData[2] = Address & 0xFF;
        TxSize = 3;
        PageSize = 32;
        break;

      case M95640:   //8Kbytes 32bytes 2bytes
      case FM25CL64: //8Kbytes    None 2bytes
        if(DataSize > (8192-Address))
          DataSize = (8192-Address);

        TxData[1] = (Address >> 8) & 0x1F;
        TxData[2] = Address & 0xFF;
        TxSize = 3;

        if(Type == FM25CL64)
          PageSize = 8192;
        else
          PageSize = 32;

        break;

      case M95128:  //16Kbytes 64bytes 2bytes
      case FM25V01: //16Kbytes    None 2bytes
        if(DataSize > (16384-Address))
          DataSize = (16384-Address);

        TxData[1] = (Address >> 8) & 0x3F;
        TxData[2] = Address & 0xFF;
        TxSize = 3;
        
        if(Type == FM25V01)
          PageSize = 16384;
        else
          PageSize = 64;
      
        break;

      case M95256:  //32Kbytes 64bytes 2bytes
      case FM25V02: //32Kbytes    None 2bytes
        if(DataSize > (32768-Address))
          DataSize = (32768-Address);

        TxData[1] = (Address >> 8) & 0x7F;
        TxData[2] = Address & 0xFF;
        TxSize = 3;
        
        if(Type == FM25V02)
          PageSize = 32768;
        else
          PageSize = 64;
        
        break;

      case M95512:  //64Kbytes 128bytes 2bytes
      case FM25V05: //64Kbytes     None 2bytes
        if(DataSize > (65536-Address))
          DataSize = (65536-Address);

        TxData[1] = (Address >> 8) & 0xFF;
        TxData[2] = Address & 0xFF;
        TxSize = 3;

        if(Type == FM25V05)
          PageSize = 65536;
        else
          PageSize = 128;

        break;

      case M95M01:  //128Kbytes 256bytes 3bytes
      case FM25V10: //128Kbytes     None 3bytes
        if(DataSize > (131072-Address))
          DataSize = (131072-Address);

        TxData[1] = (Address >> 16) & 0x01;
        TxData[2] = (Address >> 8) & 0xFF;
        TxData[3] = Address & 0xFF;
        TxSize = 4;

        if(Type == FM25V10)
          PageSize = 131072;
        else
          PageSize = 256;
        
        break;

      case M95M02:  //256Kbytes 256bytes 3bytes
      case FM25V20: //256Kbytes     None 3bytes
        if(DataSize > (262144-Address))
          DataSize = (262144-Address);

        TxData[1] = (Address >> 16) & 0x03;
        TxData[2] = (Address >> 8) & 0xFF;
        TxData[3] = Address & 0xFF;
        TxSize = 4;
        
        if(Type == FM25V20)
          PageSize = 262144;
        else
          PageSize = 256;
          
        break;

      case _25SCM04: //512Kbytes 256bytes 3bytes
      case M95M04:   //512Kbytes 512bytes 3bytes
      case FM25V40:  //512Kbytes     None 3bytes
        if(DataSize > (524288-Address))
          DataSize = (524288-Address);

        TxData[1] = (Address >> 16) & 0x07;
        TxData[2] = (Address >> 8) & 0xFF;
        TxData[3] = Address & 0xFF;
        TxSize = 4;

        if(Type == FM25V40)
          PageSize =524288;
        else if(Type == M95M04)
          PageSize =512;
        else
          PageSize =256;

        break;

      default:
        return 1;
    }

    //현재 페이지의 기록할 크기 계산
    WritableSize = PageSize - (Address % PageSize);

    if( DataSize < WritableSize)
      WritableSize = DataSize;

    //페이지 쓰기 진행
    SPIEEP_WriteEnable();

    SPIEEP_CS_LOW();
    HAL_SPI_Transmit(&SPIEEP_HANDLE, TxData, TxSize, 5);
    HAL_SPI_Transmit(&SPIEEP_HANDLE, pData, WritableSize, 10);
    SPIEEP_CS_HIGH();

    //쓰기작업 완료 대기
    SPIEEP_WaitStandbyState(5);

    //제한시간 초과 확인
    if((uwTick - Tickstart) > Timeout)
      return 2;

    //DataSize가 0일 될때까지 반복
    Address += WritableSize;
    pData += WritableSize;
    DataSize -= WritableSize;
  }

  return 0;
}
