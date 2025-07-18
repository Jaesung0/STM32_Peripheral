 /*----------------------------------------------------------------------------
  Project : STM32F1 I²C Master (Register)
  Author  : Jaesung Oh
            https://github.com/Jaesung0/STM32_Peripheral
  TEXT Encoding : UTF-8
  
  매개변수의 DevAddress는 SLA+W 형식

  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  This software is provided AS-IS.
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "i2c_MA_F1.h"
#include "delay_us.h"

//I2C 종료, StopCondition 으로 변경 및 BUSY 플레그 해제될때 까지 대기
//반환값: 정상완료 0 / 시간초과 1
static uint8_t I2C_Stop(I2C_TypeDef *I2Cx, uint32_t Tickstart, uint32_t Timeout)
{
  if((uwTick - Tickstart) >= Timeout)
    Timeout = uwTick - Tickstart + 1;

  //Generate Stop
  I2C_StopCondition(I2Cx);

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 1;
  }

  return 0;
}

static void I2C_ClearFlag_ADDR(I2C_TypeDef *I2Cx)
{
  volatile uint32_t tmpreg;
  tmpreg = I2Cx->SR1;
  tmpreg = I2Cx->SR2;
  (void)tmpreg;
}

//I2C Acknowledge Polling, 24xx 메모리 기록 사이클이 완료될때까지 대기
//반환값: 정상완료 0 / 시간초과 1
static uint8_t I2C_24xx_AckPoll(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Tickstart, uint32_t Timeout)
{
  delay_us(100);

  while(1) //Acknowledge Polling
  {
    if( !(I2C_TX_SLAW_Only(I2Cx, DevAddress, Timeout)) ) //Acknowledge Check
    {
      delay_us(10);
      return 0;
    }

    delay_us(100);

    if( ((uwTick - Tickstart) > Timeout) )
      return 1;
  }
}

//I2C Write
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 28+(23.2*DataSize)us
uint8_t I2C_Write(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, tmp_u32;

  if( (DataSize > 0) && (pData == NULL) )
    return 2;

  if(Timeout == 0)
    Timeout = 1;

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  //Disable Pos
  I2C_DisableBitPOS(I2Cx);

  //Generate Start
  I2C_StartCondition(I2Cx);

  //Wait until SB flag is set
  while( !I2C_IsActiveFlag_SB(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Send slave address
  I2C_TransmitData(I2Cx, DevAddress & 0xFE);

  //Wait until ADDR or AF flag are set
  while( !(I2C_IsActiveFlag_ADDR(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Clear ADDR flag
  I2C_ClearFlag_ADDR(I2Cx);

  //Check if the AF flag has been set
  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  if(DataSize == 0)
  {
    if( I2C_Stop(I2Cx, Tickstart, Timeout) == 0 )
      return 0;
    else
      return 3;
  }

  for(uint32_t i = 0; i < DataSize; )
  {
    I2C_TransmitData(I2Cx, *(pData + i++));

    if(i == DataSize) //마지막 바이트
    {
      while( !( (I2C_IsActiveFlag_TXE(I2Cx) && I2C_IsActiveFlag_BTF(I2Cx)) || I2C_IsActiveFlag_AF(I2Cx) ) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }
    }
    else
    {
      while( !(I2C_IsActiveFlag_TXE(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }
    }

    if(I2C_IsActiveFlag_AF(I2Cx))
    {
      I2C_ClearFlag_AF(I2Cx);
      I2C_Stop(I2Cx, Tickstart, Timeout);
      return 1;
    }
  }

  tmp_u32 = uwTick - Tickstart;

  if(Timeout <= tmp_u32)
    Timeout = 2;

  //Stop
  if( I2C_Stop(I2Cx, Tickstart, Timeout) )
    return 3;

  return 0;
}

//I2C Read
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 28+(23.2*DataSize)us
uint8_t I2C_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, DataCount = 0;

  if( (DataSize > 0) && (pData == NULL) )
    return 2;

  if(Timeout == 0)
    Timeout = 1;

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  //Disable Pos
  I2C_DisableBitPOS(I2Cx);

  //Generate Start
  I2C_StartCondition(I2Cx);

  //Wait until SB flag is set
  while( !I2C_IsActiveFlag_SB(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, Timeout);
      return 3;
    }
  }

  //Send slave address
  I2C_TransmitData(I2Cx, DevAddress | 0x01);

  //Wait until ADDR or AF flag are set
  while( !(I2C_IsActiveFlag_ADDR(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Check if the AF flag has been set
  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  switch(DataSize)
  {
    case 0:
      I2C_ClearFlag_ADDR(I2Cx);
      I2C_StopCondition(I2Cx);
      break;

    case 1:
      I2C_AcknowledgeDisable(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);
      I2C_StopCondition(I2Cx);

      //Wait until RXNE flag is set
      while( !I2C_IsActiveFlag_RXNE(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
          return 3;
      }

      //Read data from DR
      *pData = I2C_ReceiveData(I2Cx);

      //Update counter
      DataSize = 0;
      break;

    case 2:
      I2C_AcknowledgeDisable(I2Cx);
      I2C_EnableBitPOS(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);

      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Generate Stop
      I2C_StopCondition(I2Cx);

      //Read data from DR
      *pData = I2C_ReceiveData(I2Cx);
      //Read data from DR
      *(pData + 1) = I2C_ReceiveData(I2Cx);

      //Update counter
      DataSize = 0;
      break;

    default: //3이상
      I2C_AcknowledgeEnable(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);
      break;
  }

  while(DataSize - DataCount)
  {
    if( (DataSize - DataCount) == 3)
    {
      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Disable Acknowledge
      I2C_AcknowledgeDisable(I2Cx);

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Generate Stop
      I2C_StopCondition(I2Cx);

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;
      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      break;
    }
    else //4이상
    {
      //Wait until RXNE flag is set
      while( !I2C_IsActiveFlag_RXNE(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      if( I2C_IsActiveFlag_BTF(I2Cx) && ((DataSize - DataCount) > 3) )
      {
        //Read data from DR
        *(pData + DataCount) = I2C_ReceiveData(I2Cx);
        DataCount++;;
      }
    }
  }

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  return 0;
}

//I2C 디바이스주소+쓰기 후 종료
//(StartCondition → DeviceAddress+Write → StopCondition)
//반환값: ACK응답 0 / NACK응답 1 / 시간초과 3
//소요시간(400kHz): 28us
uint8_t I2C_TX_SLAW_Only(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint32_t Timeout)
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

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 2;

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  //Disable Pos
  I2C_DisableBitPOS(I2Cx);

  //Generate Start
  I2C_StartCondition(I2Cx);

  //Wait until SB flag is set
  while( !I2C_IsActiveFlag_SB(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  /* DevAddress 전송 */
  I2C_TransmitData(I2Cx, DevAddress & 0xFE);

  //Wait until ADDR or AF flag are set
  while( !(I2C_IsActiveFlag_ADDR(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Clear ADDR flag
  I2C_ClearFlag_ADDR(I2Cx);

  //Check if the AF flag has been set
  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  /* WordAddress 전송 */
  if(WordAddSize > 1)
  {
    I2C_TransmitData(I2Cx, WordAddress >> 8); //WordAddress HighByte

    while( !(I2C_IsActiveFlag_TXE(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, uwTick, 2);
        return 3;
      }
    }

    if(I2C_IsActiveFlag_AF(I2Cx))
    {
      I2C_ClearFlag_AF(I2Cx);
      I2C_Stop(I2Cx, Tickstart, Timeout);
      return 1;
    }
  }

  I2C_TransmitData(I2Cx, WordAddress & 0xFF); //WordAddress LowByte

  while( !(I2C_IsActiveFlag_TXE(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  /* Data 전송 */
  for(uint32_t i = 0; i < DataSize; )
  {
    I2C_TransmitData(I2Cx, *(pData + i++));

    if(i == DataSize) //마지막 바이트
    {
      while( !( (I2C_IsActiveFlag_TXE(I2Cx) && I2C_IsActiveFlag_BTF(I2Cx)) || I2C_IsActiveFlag_AF(I2Cx) ) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }
    }
    else
    {
      while( !(I2C_IsActiveFlag_TXE(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }
    }

    if(I2C_IsActiveFlag_AF(I2Cx))
    {
      I2C_ClearFlag_AF(I2Cx);
      I2C_Stop(I2Cx, Tickstart, Timeout);
      return 1;
    }
  }

  tmp_u32 = uwTick - Tickstart;

  if(Timeout <= tmp_u32)
    Timeout = 2;

  //Stop
  if( I2C_Stop(I2Cx, Tickstart, Timeout) )
    return 3;

  return 0;
}

//I2C memory 읽기
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
//소요시간(400kHz): 104+(23.2*DataSize)us

uint8_t I2C_Mem_Read(I2C_TypeDef *I2Cx, uint8_t DevAddress, uint16_t WordAddress, uint8_t WordAddSize, uint8_t *pData, uint32_t DataSize, uint32_t Timeout)
{
  uint32_t Tickstart = uwTick, DataCount = 0;

  if((pData == NULL) || (DataSize == 0) || (Timeout == 0))
    return 2;

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  //Disable Pos
  I2C_DisableBitPOS(I2Cx);

  //Generate Start
  I2C_StartCondition(I2Cx);

  //Wait until SB flag is set
  while( !I2C_IsActiveFlag_SB(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  /* DevAddress 전송 */
  I2C_TransmitData(I2Cx, DevAddress & 0xFE);

  //Wait until ADDR or AF flag are set
  while( !(I2C_IsActiveFlag_ADDR(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Clear ADDR flag
  I2C_ClearFlag_ADDR(I2Cx);

  //Check if the AF flag has been set
  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  /* WordAddress 전송 */
  if(WordAddSize > 1)
  {
    I2C_TransmitData(I2Cx, WordAddress >> 8); //WordAddress HighByte

    while( !(I2C_IsActiveFlag_TXE(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
    {
      if((uwTick - Tickstart) > Timeout)
      {
        I2C_Stop(I2Cx, uwTick, 2);
        return 3;
      }
    }

    if(I2C_IsActiveFlag_AF(I2Cx))
    {
      I2C_ClearFlag_AF(I2Cx);
      I2C_Stop(I2Cx, Tickstart, Timeout);
      return 1;
    }
  }

  I2C_TransmitData(I2Cx, WordAddress & 0xFF); //WordAddress LowByte

  while( !( (I2C_IsActiveFlag_TXE(I2Cx) && I2C_IsActiveFlag_BTF(I2Cx)) || I2C_IsActiveFlag_AF(I2Cx) ) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  /* Repeated Start */
  I2C_StartCondition(I2Cx);

  //Wait until SB flag is set
  while( !I2C_IsActiveFlag_SB(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, Timeout);
      return 3;
    }
  }

  /* DevAddress 전송 */
  I2C_TransmitData(I2Cx, DevAddress | 0x01);

  //Wait until ADDR or AF flag are set
  while( !(I2C_IsActiveFlag_ADDR(I2Cx) || I2C_IsActiveFlag_AF(I2Cx)) )
  {
    if((uwTick - Tickstart) > Timeout)
    {
      I2C_Stop(I2Cx, uwTick, 2);
      return 3;
    }
  }

  //Check if the AF flag has been set
  if(I2C_IsActiveFlag_AF(I2Cx))
  {
    I2C_ClearFlag_AF(I2Cx);
    I2C_Stop(I2Cx, Tickstart, Timeout);
    return 1;
  }

  /* Data Read */
  switch(DataSize)
  {
    case 1:
      I2C_AcknowledgeDisable(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);
      I2C_StopCondition(I2Cx);

      //Wait until RXNE flag is set
      while( !I2C_IsActiveFlag_RXNE(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
          return 3;
      }

      //Read data from DR
      *pData = I2C_ReceiveData(I2Cx);

      //Update counter
      DataSize = 0;
      break;

    case 2:
      I2C_AcknowledgeDisable(I2Cx);
      I2C_EnableBitPOS(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);

      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Generate Stop
      I2C_StopCondition(I2Cx);

      //Read data from DR
      *pData = I2C_ReceiveData(I2Cx);
      //Read data from DR
      *(pData + 1) = I2C_ReceiveData(I2Cx);

      //Update counter
      DataSize = 0;
      break;

    default: //3이상
      I2C_AcknowledgeEnable(I2Cx);
      I2C_ClearFlag_ADDR(I2Cx);
      break;
  }

  while(DataSize - DataCount)
  {
    if( (DataSize - DataCount) == 3)
    {
      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Disable Acknowledge
      I2C_AcknowledgeDisable(I2Cx);

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      //Wait until BTF flag is set
      while( !I2C_IsActiveFlag_BTF(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Generate Stop
      I2C_StopCondition(I2Cx);

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;
      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      break;
    }
    else //4이상
    {
      //Wait until RXNE flag is set
      while( !I2C_IsActiveFlag_RXNE(I2Cx) )
      {
        if((uwTick - Tickstart) > Timeout)
        {
          I2C_Stop(I2Cx, uwTick, 2);
          return 3;
        }
      }

      //Read data from DR
      *(pData + DataCount) = I2C_ReceiveData(I2Cx);
      DataCount++;

      if( I2C_IsActiveFlag_BTF(I2Cx) && ((DataSize - DataCount) > 3) )
      {
        //Read data from DR
        *(pData + DataCount) = I2C_ReceiveData(I2Cx);
        DataCount++;;
      }
    }
  }

  //Wait until BUSY flag is reset
  while( I2C_IsActiveFlag_BUSY(I2Cx) )
  {
    if((uwTick - Tickstart) > Timeout)
      return 3;
  }

  return 0;
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
  uint8_t DevAddress, WordAddSize, return_val;

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
  uint16_t WordAddWrite;
  uint8_t DevAddress, WordAddSize;

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

//I2C EEPROM 전체를 특정값으로 채우기
//  DevCapacity: EEPROM 용량
//  ChipAddress: A0,A1,A2 핀 연결상태(일부제조사의 24C04, 24C08 은 A0,A1,A2 핀모두가 Don't care)
//반환값: 정상완료 0 / NACK응답 1 / 매개변수오류 2 / 시간초과 3
uint8_t I2C_24xx_Fill(I2C_TypeDef *I2Cx, uint16_t DevCapacity, uint8_t ChipAddress, uint8_t Fill)
{
  uint16_t NumOfPage, PageSize, WordAdd = 0;
  uint8_t DevAddress, NumOfDevAdd = 1, WordAddSize, return_val;;
  uint8_t Data[256];

  switch (DevCapacity)
  {
    case 0:    //  16bytes,   PAGE 쓰기 없음,      ChipAddress: 없음,     WordAddress:              A3~A0
      DevAddress = 0xA0;
      WordAddSize = 1;
      NumOfDevAdd = 1;
      NumOfPage = 16;
      PageSize = 1;
      break;

    case 1:    // 128Bytes,   16pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A6~A0, (일부제조사는 16Bytes 크기의 Page)
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 1;
      NumOfDevAdd = 1;
      NumOfPage = 16;
      PageSize = 8;
      break;

    case 2:    // 256Bytes,   32pages of  8bytes,  ChipAddress: A2 A1 A0, WordAddress:              A7~A0, (일부제조사는 16Bytes 크기의 Page)
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 1;
      NumOfDevAdd = 1;
      NumOfPage = 32;
      PageSize = 8;
      break;

    case 4:    // 512Bytes,   32pages of 16bytes,  ChipAddress: A2 A1,    WordAddress:    B0        A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1);
      WordAddSize = 1;
      NumOfDevAdd = 2;
      NumOfPage = 32/NumOfDevAdd;
      PageSize = 16;
      break;

    case 8:    //  1Kbytes,   64pages of 16bytes,  ChipAddress: A2,       WordAddress: B1~B0        A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1);
      WordAddSize = 1;
      NumOfDevAdd = 4;
      NumOfPage = 64/NumOfDevAdd;
      PageSize = 16;
      break;

    case 16:   //  2Kbytes,  128pages of 16bytes,  ChipAddress: 없음,     WordAddress: B2~B0        A7~A0
      DevAddress = 0xA0;
      WordAddSize = 1;
      NumOfDevAdd = 8;
      NumOfPage = 128/NumOfDevAdd;
      PageSize = 16;
      break;

    case 32:   //  4Kbytes,  128pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A11~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 1;
      NumOfPage = 128;
      PageSize = 32;
      break;

    case 64:   //  8Kbytes,  256pages of 32bytes,  ChipAddress: A2 A1 A0, WordAddress:       A12~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 1;
      NumOfPage = 256;
      PageSize = 32;
      break;

    case 128:  // 16Kbytes,  256pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A13~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 1;
      NumOfPage = 256;
      PageSize = 64;
      break;

    case 256:  // 32Kbytes,  512pages of 64bytes,  ChipAddress: A2 A1 A0, WordAddress:       A14~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 1;
      NumOfPage = 512;
      PageSize = 64;
      break;

    case 512:  // 64Kbytes,  512pages of 128bytes, ChipAddress: A2 A1 A0, WordAddress:       A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x07)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 1;
      NumOfPage = 512;
      PageSize = 128;
      break;

    case 1024: //128Kbytes,  512pages of 256bytes, ChipAddress: A2 A1,    WordAddress:    B0 A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x06)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 2;
      NumOfPage = 512/NumOfDevAdd;
      PageSize = 256;
      break;

    case 2048: //256Kbytes, 1024pages of 256bytes, ChipAddress: A2,       WordAddress: B1~B0 A15~A8 A7~A0
      DevAddress = 0xA0 | ((ChipAddress & 0x04)<<1);
      WordAddSize = 2;
      NumOfDevAdd = 4;
      NumOfPage = 1024/NumOfDevAdd;
      PageSize = 256;
      break;

    default:
      return 2;
  }

  for(uint16_t i = 0; i < PageSize; i++)
    Data[i] = Fill;

  for(uint8_t j = 0; j < NumOfDevAdd; j++)
  {
    DevAddress = 0xA0 | (j<<1);

    for(uint16_t i = 0; i < NumOfPage; i++)
    {
      WordAdd = PageSize * i;

      return_val = I2C_Mem_Write(I2Cx, DevAddress, WordAdd, WordAddSize, Data, PageSize, 50);

      if(return_val)
        return return_val;

      if( I2C_24xx_AckPoll(I2Cx, DevAddress, uwTick, 10) )
        return 3;
    }
  }

  return 0;
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
