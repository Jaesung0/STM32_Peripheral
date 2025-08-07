/*----------------------------------------------------------------------------
  Project : STM32F7 I²C Master (Register)
  Author  : Jaesung Oh
            https://github.com/Jaesung0/STM32_Peripheral
  TEXT Encoding : UTF-8
  
  매개변수의 DevAddress는 SLA+W 형식

  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause

  The source code and the binary form, 
  and any modifications made to them may not be used for the purpose of training or improving machine learning algorithms,
  including but not limited to artificial intelligence, natural language processing, or data mining. 
  This condition applies to any derivatives, modifications, or updates based on the Software code. 
  Any usage of the source code or the binary form in an AI-training dataset is considered a breach of this License.
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "i2c_F7.h"
#include "delay_us.h"

//I2C 종료, StopCondition 으로 변경 및 BUSY 플레그 해제될때 까지 대기
//반환값: 정상완료 0 / 시간초과 1
static uint8_t I2C_Stop(I2C_TypeDef *I2Cx, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick;

  if(Timeout == 0)
    Timeout = 1;

  I2C_StopCondition(I2Cx);

  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_ClearFlag_ALL(I2Cx);
      return 1;
    }
  }

  I2C_ClearFlag_ALL(I2Cx);
  return 0;
}

//I2C Write
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 28+(23.2*DataSize)us
uint8_t I2C_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, tmp_u32;
  uint32_t i;

  if( ((DataSize > 0) && (pData == NULL)) || (Timeout == 0) )
    return 2;

  if(DataSize < 256)
    I2Cx->CR2 = (DevAddress & 0xFE) | (DataSize<<16); //Write, NBYTES=DataSize
  else
    I2Cx->CR2 = (DevAddress & 0xFE) | (255<<16) | (1<<24); //Write, NBYTES=255, NBYTES Reload

  I2C_StartCondition(I2Cx);

  //ADD+W 전송완료 대기
  if( DataSize == 0 )
  {
    while( !(I2C_IsActiveFlag_TC(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, 2);
        return 3;
      }
    }
  }
  else
  {
    while( !(I2C_IsActiveFlag_TXIS(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, 2);
        return 3;
      }
    }
  }

  if( I2C_IsActiveFlag_NACK(I2Cx) )
  {
    I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
    return 1;
  }

  for(i=0 ; i<DataSize;)
  {
    I2C_TransmitData(I2Cx, *(pData + i++));

    if(i == DataSize)
    {
      while( !(I2C_IsActiveFlag_TC(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )//마지막 바이트
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, 2);
          return 3;
        }
      }
    }
    else
    {
      while( !(I2C_IsActiveFlag_TCR(I2Cx) || I2C_IsActiveFlag_TXIS(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, 2);
          return 3;
        }
      }
    }

    if( I2C_IsActiveFlag_NACK(I2Cx) )
    {
      I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
      return 1;
    }

    if( I2C_IsActiveFlag_TCR(I2Cx) )
    {
      if((DataSize-i) < 256)
      {
        I2C_SetTransferSize(I2Cx, DataSize-i);
        I2C_DisableReloadMode(I2Cx);
      }
      else
        I2C_SetTransferSize(I2Cx, 255);
    }
  }

  tmp_u32 = uwTick - Tickstart;

  if(Timeout <= tmp_u32)
    Timeout = 2;

  if( I2C_Stop(I2Cx, Timeout) )
    return 3;
  else
    return 0;
}

//I2C Read
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 28+(23.2*DataSize)us
uint8_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, tmp_u32;
  uint32_t i;

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 2;

  if(DataSize < 256)
    I2Cx->CR2 = (DevAddress & 0xFE) | (1<<10) | (DataSize<<16); //Read, NBYTES=DataSize
  else
    I2Cx->CR2 = (DevAddress & 0xFE) | (1<<10) | (255<<16) | (1<<24); //Read, NBYTES=255, NBYTES Reload

  I2C_StartCondition(I2Cx);

  for(i=0 ; i<DataSize;)
  {
    while( !(I2C_IsActiveFlag_TCR(I2Cx) || I2C_IsActiveFlag_RXNE(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, 2);
        return 3;
      }
    }

    if( I2C_IsActiveFlag_NACK(I2Cx) )
    {
      I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
      *pData = 0xFF;
      return 1;
    }

    *(pData + i++) = I2C_ReceiveData(I2Cx); //Read data

    if( I2C_IsActiveFlag_TCR(I2Cx) )
    {
      if( (DataSize-i) < 255 )
      {
        I2C_SetTransferSize(I2Cx, DataSize-i);
        I2C_DisableReloadMode(I2Cx);
      }
      else
        I2C_SetTransferSize(I2Cx, 255);
    }
  }

  while( !(I2C_IsActiveFlag_TC(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, 2);
      return 3;
    }
  }

  tmp_u32 = uwTick - Tickstart;

  if(Timeout <= tmp_u32)
    Timeout = 2;

  if( I2C_Stop(I2Cx, Timeout) )
    return 3;
  else
    return 0;
}

//I2C 디바이스주소+쓰기 후 종료
//(StartCondition → DeviceAddress+Write → StopCondition)
//반환값: ACK응답 0 / NACK응답 1 / 시간초과 3
//소요시간(400kHz): 28us
uint8_t I2C_TX_ADDW_Only(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Timeout)
{
  return I2C_Write(I2Cx, DevAddress, NULL, 0, Timeout);
}

//I2C memory 워드주소 쓰기 후 종료
//(StartCondition → DeviceAddress+Write → WordAddress → StopCondition)
//반환값: ACK응답 0 / NACK응답 1 / 시간초과 3
//소요시간(400kHz): 75us
uint8_t I2C_Mem_Address_Set(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint32_t Timeout)
{
  uint8_t data[2];

  data[0] = WordAddress >> 8;
  data[1] = WordAddress & 0xFF;

  if(WordAddSize < 2)
    return I2C_Write(I2Cx, DevAddress, &data[1], 1, Timeout);
  else
    return I2C_Write(I2Cx, DevAddress, data, 2, Timeout);
}

//I2C memory 현재주소 읽기
//(StartCondition → DeviceAddress+Read → DataRead → StopCondition)
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 52us
uint8_t I2C_Mem_Current_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t Timeout)
{
  return I2C_Read(I2Cx, DevAddress, pData, 1, Timeout);
}

//I2C memory 쓰기 
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 74+(23.2*DataSize)us
uint8_t I2C_Mem_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, tmp_u32;
  uint32_t i;

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 2;

  /* WordAddress 전송 */
  if(WordAddSize < 2)
    I2Cx->CR2 = (DevAddress & 0xFE) | (1<<16) | (1<<24); //Write, NBYTES=1, NBYTES Reload
  else
    I2Cx->CR2 = (DevAddress & 0xFE) | (2<<16) | (1<<24); //Write, NBYTES=2, NBYTES Reload

  I2C_StartCondition(I2Cx);

  while( !(I2C_IsActiveFlag_TXIS(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) ) //ADD+W 전송완료 대기
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, 2);
      return 3;
    }
  }

  if( I2C_IsActiveFlag_NACK(I2Cx) )
  {
    I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
    return 1;
  }

  if(WordAddSize > 1)
  {
    I2C_TransmitData(I2Cx, WordAddress >> 8); //WordAddress HighByte

    while( !(I2C_IsActiveFlag_TXIS(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, 2);
        return 3;
      }
    }

    if( I2C_IsActiveFlag_NACK(I2Cx) )
    {
      I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
      return 1;
    }
  }

  I2C_TransmitData(I2Cx, WordAddress & 0xFF); //WordAddress LowByte

  while( !(I2C_IsActiveFlag_TCR(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, 2);
      return 3;
    }
  }

  if( I2C_IsActiveFlag_NACK(I2Cx) )
  {
    I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
    return 1;
  }

  /* Data 전송 */
  if(DataSize < 256)
  {
    I2C_SetTransferSize(I2Cx, DataSize);
    I2C_DisableReloadMode(I2Cx);
  }
  else
    I2C_SetTransferSize(I2Cx, 255);

  for(i=0 ; i<DataSize;)
  {
    I2C_TransmitData(I2Cx, *(pData + i++));

    if(i == DataSize)
    {
      while( !(I2C_IsActiveFlag_TC(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )//마지막 바이트
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, 2);
          return 3;
        }
      }
    }
    else
    {
      while( !(I2C_IsActiveFlag_TCR(I2Cx) || I2C_IsActiveFlag_TXIS(I2Cx) || I2C_IsActiveFlag_NACK(I2Cx)) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, 2);
          return 3;
        }
      }
    }

    if( I2C_IsActiveFlag_NACK(I2Cx) )
    {
      I2C_Stop(I2Cx, 2); //I2C_ClearFlag_NACK(I2Cx);
      return 1;
    }

    if( I2C_IsActiveFlag_TCR(I2Cx) )
    {
      if((DataSize-i) < 256)
      {
        I2C_SetTransferSize(I2Cx, DataSize-i);
        I2C_DisableReloadMode(I2Cx);
      }
      else
        I2C_SetTransferSize(I2Cx, 255);
    }
  }

  tmp_u32 = uwTick - Tickstart;

  if(Timeout <= tmp_u32)
    Timeout = 2;

  if( I2C_Stop(I2Cx, Timeout) )
    return 3;
  else
    return 0;
}

//I2C memory 읽기
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 104+(23.2*DataSize)us
uint8_t I2C_Mem_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick;
  uint8_t return_val;

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 2;

  return_val = I2C_Mem_Address_Set(I2Cx, DevAddress, WordAddress, WordAddSize, Timeout);

  if(return_val)
    return return_val;

  Timeout =  Timeout - (uwTick - Tickstart);

  return_val = I2C_Read(I2Cx, DevAddress, pData, DataSize, Timeout);

  return return_val;
}

//I2C Acknowledge Polling, 24xx 메모리 기록 사이클이 완료될때까지 대기
//반환값: 정상완료 0 / 시간초과 1
uint8_t I2C_24xx_AckPoll(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Tickstart, uint32_t Timeout)
{
  while(1) //Acknowledge Polling
  {
    //while( !(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) ); //1ms씩 대기,첫번째 실행시에는 대기하지 않음

    if( !(I2C_TX_ADDW_Only(I2Cx, DevAddress, Timeout)) ) //Acknowledge Check
      return 0;

    delay_us(100);

    if( ((uwTick - Tickstart) > Timeout) )
      return 1;
  }
}

//I2C EEPROM 쓰기
//매개변수:
//  DevCapacity: EEPROM 용량
//  ChipAddress: A0,A1,A2 핀 연결상태(일부제조사의 24C04, 24C08 은 A0,A1,A2 핀모두가 Don't care)
//  Timeout: 제한시간(page 당 5ms 소요됨 고려)
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
uint8_t I2C_24xx_Write(I2C_TypeDef *I2Cx, uint16_t DevCapacity, uint8_t ChipAddress, uint32_t WordAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick;
  uint16_t PageSize, WritableSize, WordAddWrite;
  uint8_t DevAddress, WordAddSize;
  uint8_t return_val;

  if((pData == NULL) || (Timeout == 0))
    return 2;

  while(DataSize)
  {
    switch (DevCapacity)
    {
      case 0:    //  16bytes,   PAGE 쓰기 없음,      ChipAddress: 없음,     WordAddress:              A3~A0
        DevAddress = 0xA0;
        WordAddWrite = WordAddress & 0x0F;
        WordAddSize = 1;
        PageSize = 1;
        if(DataSize > (16-WordAddress))
          DataSize = (16-WordAddress);
        break;

      case 1:    // 128Bytes,   16pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A6~A0, (일부제조사는 16Bytes 크기의 Page)
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0x7F;
        WordAddSize = 1;
        PageSize = 8;
        if(DataSize > (128-WordAddress))
          DataSize = (128-WordAddress);
        break;

      case 2:    // 256Bytes,   32pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A7~A0, (일부제조사는 16Bytes 크기의 Page)
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0xFF;
        WordAddSize = 1;
        PageSize = 8;
        if(DataSize > (256-WordAddress))
          DataSize = (256-WordAddress);
        break;

      case 4:    // 512Bytes,   32pages of 16bytes,  ChipAddress: A2 A1,    WordAddress:    B0        A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1) | ((WordAddress >> 7) & 0x02);
        WordAddWrite = WordAddress & 0xFF;
        WordAddSize = 1;
        PageSize = 16;
        if(DataSize > (512-WordAddress))
          DataSize = (512-WordAddress);
        break;

      case 8:    //  1Kbytes,   64pages of 16bytes,  ChipAddress: A2,       WordAddress: B1~B0        A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1) | ((WordAddress >> 7) & 0x06);
        WordAddWrite = WordAddress & 0xFF;
        WordAddSize = 1;
        PageSize = 16;
        if(DataSize > (1024-WordAddress))
          DataSize = (1024-WordAddress);
        break;

      case 16:   //  2Kbytes,  128pages of 16bytes,  ChipAddress: 없음,     WordAddress: B2~B0        A7~A0
        DevAddress = 0xA0 | ((WordAddress >> 7) & 0x0E);
        WordAddWrite = WordAddress & 0xFF;
        WordAddSize = 1;
        PageSize = 16;
        if(DataSize > (2048-WordAddress))
          DataSize = (2048-WordAddress);
        break;

      case 32:   //  4Kbytes,  128pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A11~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0x0FFF;
        WordAddSize = 2;
        PageSize = 32;
        if(DataSize > (4096-WordAddress))
          DataSize = (4096-WordAddress);
        break;

      case 64:   //  8Kbytes,  256pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A12~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0x1FFF;
        WordAddSize = 2;
        PageSize = 32;
        if(DataSize > (8192-WordAddress))
          DataSize = (8192-WordAddress);
        break;

      case 128:  // 16Kbytes,  256pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A13~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0x3FFF;
        WordAddSize = 2;
        PageSize = 64;
        if(DataSize > (16384-WordAddress))
          DataSize = (16384-WordAddress);
        break;

      case 256:  // 32Kbytes,  512pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A14~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0x7FFF;
        WordAddSize = 2;
        PageSize = 64;
        if(DataSize > (32768-WordAddress))
          DataSize = (32768-WordAddress);
        break;

      case 512:  // 64Kbytes,  512pages of 128bytes, ChipAddress: A2 A1 A0, WordAddress:       A15~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
        WordAddWrite = WordAddress & 0xFFFF;
        WordAddSize = 2;
        PageSize = 128;
        if(DataSize > (65536-WordAddress))
          DataSize = (65536-WordAddress);
        break;

      case 1024: //128Kbytes,  512pages of 256bytes, ChipAddress: A2 A1,    WordAddress:    B0 A15~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1) | ((WordAddress >> 15) & 0x02);
        WordAddWrite = WordAddress & 0xFFFF;
        WordAddSize = 2;
        PageSize = 256;
        if(DataSize > (131072-WordAddress))
          DataSize = (131072-WordAddress);
        break;

      case 2048: //256Kbytes, 1024pages of 256bytes, ChipAddress: A2,       WordAddress: B1~B0 A15~A8 A7~A0
        DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1) | ((WordAddress >> 15) & 0x06);
        WordAddWrite = WordAddress & 0xFFFF;
        WordAddSize = 2;
        PageSize = 256;
        if(DataSize > (262144-WordAddress))
          DataSize = (262144-WordAddress);
        break;

      default:
        return 2;
    }

    WritableSize = PageSize - (WordAddress % PageSize); //현재 페이지의 기록가능한 크기 최대값 계산

    if( DataSize < WritableSize)
      WritableSize = DataSize;

    return_val = I2C_Mem_Write(I2Cx, DevAddress, WordAddWrite, WordAddSize, pData, WritableSize, Timeout);

    if(return_val)
      return return_val;

    if( I2C_24xx_AckPoll(I2Cx, DevAddress, Tickstart, Timeout) )
      return 3;

    WordAddress += WritableSize;
    pData += WritableSize;
    DataSize -= WritableSize;
  }

  return 0;
}

//I2C 24xx EEPROM 읽기
//매개변수:
//  DevCapacity: EEPROM 용량
//  ChipAddress: A0,A1,A2 핀 연결상태(일부제조사의 24C04, 24C08 은 A0,A1,A2 핀모두가 Don't care)
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 104+(23.2*DataSize)us
uint8_t I2C_24xx_Read( I2C_TypeDef *I2Cx, uint16_t DevCapacity, uint8_t ChipAddress, uint32_t WordAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint8_t DevAddress, WordAddSize;
  uint16_t WordAddWrite;

  if((pData == NULL) || (DataSize == 0))
    return 2;

  switch (DevCapacity)
  {
    case 0:    //  16bytes,   PAGE 쓰기 없음,      ChipAddress: 없음,     WordAddress:              A3~A0
      DevAddress = 0xA0;
      WordAddWrite = WordAddress & 0x0F;
      WordAddSize = 1;
      if(DataSize>16) DataSize=16;
      break;

    case 1:    // 128Bytes,   16pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A6~A0, (일부제조사는 16Bytes 크기의 Page)
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0x7F;
      WordAddSize = 1;
      if(DataSize>128) DataSize=128;
      break;

    case 2:    // 256Bytes,   32pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A7~A0, (일부제조사는 16Bytes 크기의 Page)
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0xFF;
      WordAddSize = 1;
      if(DataSize>256) DataSize=256;
      break;

    case 4:    // 512Bytes,   32pages of 16bytes,  ChipAddress: A2 A1,    WordAddress:    B0        A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1) | ((WordAddress >> 7) & 0x02);
      WordAddWrite = WordAddress & 0xFF;
      WordAddSize = 1;
      if(DataSize>512) DataSize=512;
      break;

    case 8:    //  1Kbytes,   64pages of 16bytes,  ChipAddress: A2,       WordAddress: B1~B0        A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1) | ((WordAddress >> 7) & 0x06);
      WordAddWrite = WordAddress & 0xFF;
      WordAddSize = 1;
      if(DataSize>1024) DataSize=1024;
      break;

    case 16:   //  2Kbytes,  128pages of 16bytes,  ChipAddress: 없음,     WordAddress: B2~B0        A7~A0
      DevAddress = 0xA0 | ((WordAddress >> 7) & 0x0E);
      WordAddWrite = WordAddress & 0xFF;
      WordAddSize = 1;
      if(DataSize>2048) DataSize=2048;
      break;

    case 32:   //  4Kbytes,  128pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A11~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0x0FFF;
      WordAddSize = 2;
      if(DataSize>4096) DataSize=4096;
      break;

    case 64:   //  8Kbytes,  256pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A12~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0x1FFF;
      WordAddSize = 2;
      if(DataSize>8192) DataSize=8192;
      break;

    case 128:  // 16Kbytes,  256pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A13~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0x3FFF;
      WordAddSize = 2;
      if(DataSize>16384) DataSize=16384;
      break;

    case 256:  // 32Kbytes,  512pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A14~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0x7FFF;
      WordAddSize = 2;
      if(DataSize>32768) DataSize=32768;
      break;

    case 512:  // 64Kbytes,  512pages of 128bytes, ChipAddress: A2 A1 A0, WordAddress:       A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddWrite = WordAddress & 0xFFFF;
      WordAddSize = 2;
      if(DataSize>65536) DataSize=65536;
      break;

    case 1024: //128Kbytes,  512pages of 256bytes, ChipAddress: A2 A1,    WordAddress:    B0 A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1) | ((WordAddress >> 15) & 0x02);
      WordAddWrite = WordAddress & 0xFFFF;
      WordAddSize = 2;
      if(DataSize>131072) DataSize=131072;
      break;

    case 2048: //256Kbytes, 1024pages of 256bytes, ChipAddress: A2,       WordAddress: B1~B0 A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1) | ((WordAddress >> 15) & 0x06);
      WordAddWrite = WordAddress & 0xFFFF;
      WordAddSize = 2;
      if(DataSize>262144) DataSize=262144;
      break;

    default:
      return 2;
  }

  return I2C_Mem_Read(I2Cx, DevAddress, WordAddWrite, WordAddSize, pData, DataSize, Timeout);
}

//I2C Transmit General Call(SecondByte=0x06 Reset, SecondByte=0x04 Latch ID)
//반환값: ACK응답 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 52us
uint8_t I2C_TX_General_Call(I2C_TypeDef *I2Cx, uint8_t SecondByte, uint32_t Timeout)
{
  if(SecondByte == 0x00)
    return 2;

  return I2C_Write(I2Cx, 0x00, &SecondByte, 1, Timeout);
}

/*i2c 통신함수 예제
  {
    uint8_t tmp_u8;
    uint8_t i2c_write[1024] = {0,};
    uint8_t i2c_read[1024] = {0,};

    tmp_u8 = I2C_TX_ADDW_Only(I2C1, 0xA0, 1);
    printf("I2C_TX_ADDW_Only: ADDR = 0xA0, Return = %u\r\n", tmp_u8);

    tmp_u8 = I2C_Mem_Address_Set(I2C1, 0xA0, 0x00, 1, 1);
    printf("I2C_Mem_Address_Set = 0x00, ADDR = 0xA0, Return = %u\r\n",tmp_u8);

    tmp_u8 = I2C_Mem_Current_Read(I2C1, 0xA0, &i2c_read[0], 1);
    printf("I2C_Mem_Current_Read: ADDR = 0xA0, DATA = 0x%02X Return = %u\r\n", i2c_read[0], tmp_u8);

    tmp_u8 = I2C_TX_General_Call(I2C1, 0x04, 1);
    printf("I2C_TX_General_Call: Return = %u\r\n",tmp_u8);

    tmp_u8 = I2C_Write(I2C1, 0xA0, &i2c_write[0], 128, 10);
    printf("I2C_Write 128Byte,  Return = %u\r\n",tmp_u8);
    HAL_Delay(5);

    tmp_u8 = I2C_Read(I2C1, 0xA0, &i2c_read[0], 128, 10);
    printf("I2C_Read 128Byte,  Return = %u\r\n",tmp_u8);

    tmp_u8 = I2C_Mem_Write(I2C1, 0xA0, 0, 1, &i2c_write[0], 128, 10);
    printf("I2C_Mem_Write 128Byte,  Return = %u\r\n",tmp_u8);
    HAL_Delay(5);

    tmp_u8 = I2C_Mem_Read(I2C1, 0xA0, 0, 1, &i2c_read[0], 128, 10);
    printf("I2C_Mem_Read 128Byte,  Return = %u\r\n",tmp_u8);

    for(uint16_t i=0; i<1024;i++)
      i2c_write[i] = i;

    tmp_u8 = I2C_24xx_Write(I2C1, 2, 0, 0x8, &i2c_write[8], 8, 10);
    printf("I2C_24xx_Write 8Byte,  Return = %u\r\n",tmp_u8);

    tmp_u8 = I2C_24xx_Read(I2C1, 2, 0, 0, &i2c_read[0], 128, 10);
    printf("I2C_24xx_Read 128Byte,  Return = %u\r\n",tmp_u8);

    for(uint16_t i=0; i<1024;i++)
      i2c_write[i] = 1023-i;

    tmp_u8 = I2C_24xx_Write(I2C4, 64, 0, 0x00, &i2c_write[0], 1024, 200);
    printf("I2C_24xx_Write,  Return = %u\r\n",tmp_u8);

    tmp_u8 = I2C_24xx_Read(I2C4, 64, 0, 0x00, &i2c_read[0], 1024, 30);
    printf("I2C_24xx_Read,  Return = %u\r\n",tmp_u8);
  }
*/

