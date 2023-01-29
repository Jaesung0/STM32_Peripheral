 /*----------------------------------------------------------------------------
  Project :  STM32F1 I²C Master (Register)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8

  매개변수의 DevAddress는 SLA+W 형식
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License. 
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "i2c_F1.h"
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

//I2C_ClearBusyFlagErratum()의 종속함수
// 매개변수
//  0: I2C1, I2C1_REMAP=0
//  1: I2C1, I2C1_REMAP=1
//  2: I2C2
static void I2C_ClearBusyFlagHelper(uint8_t type)
{
  uint32_t Tickstart, tmp_u32;

  //1. I2Cx_CR1 레지스터의 PE 비트를 크리어하여 I2C를 비활성화.
  //2. SCL 및 SDA I/O를 범용 출력 오픈 드레인, 하이 레벨로 구성(GPIOx_ODR에 1쓰기).
  switch(type)
  {
    case 0: //SCL: PB6, SDA: PB7
      I2C_Disable(I2C1);

      tmp_u32 = GPIOB->CRL;    // | b31 | b30 | b29 | b28  | b27 | b26 | b25 | b24  |
      tmp_u32 &= 0x00FFFFFFUL; // | CNF7[1:0] | MODE7[1:0] | CNF6[1:0] | MODE6[1:0] |
      tmp_u32 |= 0x77000000UL; // |  0  |  1  |  1  |  1   |  0  |  1  |  1  |  1   |
      GPIOB->CRL = tmp_u32;

      tmp_u32 = 0x000000C0UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 1: //SCL: PB8, SDA: PB9
      I2C_Disable(I2C1);

      tmp_u32 = GPIOB->CRH;    // | b7  | b6  | b5  | b4   | b3  | b2  | b1  | b0   |
      tmp_u32 &= 0xFFFFFF00UL; // | CNF9[1:0] | MODE9[1:0] | CNF8[1:0] | MODE8[1:0] |
      tmp_u32 |= 0x00000077UL; // |  0  |  1  |  1  |  1   |  0  |  1  |  1  |  1   |
      GPIOB->CRH = tmp_u32;

      tmp_u32 = 0x00000300UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 2: //SCL: PB10, SDA: PB11
      I2C_Disable(I2C2);

      tmp_u32 = GPIOB->CRH;    // | b15  | b14 | b13  | b12  | b11  | b10 |  b9  |  b8  |
      tmp_u32 &= 0xFFFF00FFUL; // | CNF11[1:0] | MODE11[1:0] | CNF10[1:0] | MODE10[1:0] |
      tmp_u32 |= 0x00007700UL; // |  0   |  1  |  1   |  1   |  0   |  1  |  1   |  1   |
      GPIOB->CRH = tmp_u32;

      tmp_u32 = 0x00000C00UL;
      GPIOB->ODR |= tmp_u32;
      break;

    default:
      return;
  }

  //3. GPIOx_IDR에서 SCL 및 SDA High 레벨 확인.
  Tickstart = uwTick;

  while( (GPIOB->IDR & tmp_u32) != tmp_u32 )
  {
    if((uwTick - Tickstart) > 5)
      break;
  }

  //4. SDA I/O를 범용 출력 오픈 드레인, 로우 레벨로 구성(GPIOx_ODR에 0 쓰기).
  switch(type)
  {
    default:
    case 0: //SDA: PB7
      GPIOB->ODR &= 0xFFFFFF7FUL;
      tmp_u32 = 0x00000080UL;
      break;

    case 1: //SDA: PB9
      GPIOB->ODR &= 0xFFFFFDFFUL;
      tmp_u32 = 0x00000200UL;
      break;

    case 2: //SDA: PB11
      GPIOB->ODR &= 0xFFFFF7FFUL;
      tmp_u32 = 0x00000800UL;
      break;
  }

  //5. GPIOx_IDR에서 SDA 로우 레벨 확인
  Tickstart = uwTick;

  while( (GPIOB->IDR & tmp_u32) )
  {
    if((uwTick - Tickstart) > 5)
      break;
  }

  //6. SCL I/O를 범용 출력 오픈 드레인, 로우 레벨로 구성(GPIOx_ODR에 0 쓰기).
  switch(type)
  {
    default:
    case 0: //SCL: PB6
      GPIOB->ODR &= 0xFFFFFFBFUL;
      tmp_u32 = 0x00000040UL;
      break;

    case 1: //SCL: PB8
      GPIOB->ODR &= 0xFFFFFEFFUL;
      tmp_u32 = 0x00000100UL;
      break;

    case 2: //SCL: PB10
      GPIOB->ODR &= 0xFFFFFBFFUL;
      tmp_u32 = 0x00000400UL;
      break;
  }

  //7. GPIOx_IDR에서 SCL 로우 레벨 확인
  Tickstart = uwTick;

  while( (GPIOB->IDR & tmp_u32) )
  {
    if((uwTick - Tickstart) > 5)
      break;
  }

  //8. SCL I/O를 범용 출력 오픈 드레인, 하이 레벨로 구성(GPIOx_ODR에 1 쓰기).
  switch(type)
  {
    default:
    case 0: //SCL: PB6
      //tmp_u32 = 0x00000040UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 1: //SCL: PB8
      //tmp_u32 = 0x00000100UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 2: //SCL: PB10
      //tmp_u32 = 0x00000400UL;
      GPIOB->ODR |= tmp_u32;
      break;
  }

  //9. GPIOx_IDR에서 SCL 하이 레벨 확인
  Tickstart = uwTick;

  while( (GPIOB->IDR & tmp_u32) !=  tmp_u32)
  {
    if((uwTick - Tickstart) > 5)
      break;
  }

  //10. SDA I/O를 범용 출력 오픈 드레인, 하이 레벨로 구성(GPIOx_ODR에 1 쓰기).
  switch(type)
  {
    default:
    case 0: //SDA: PB7
      tmp_u32 = 0x00000080UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 1: //SDA: PB9
      tmp_u32 = 0x00000200UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 2: //SDA: PB11
      tmp_u32 = 0x00000800UL;
      GPIOB->ODR |= tmp_u32;
      break;
  }

  //11. GPIOx_IDR에서 SCL 하이 레벨 확인
  Tickstart = uwTick;

  while( (GPIOB->IDR & tmp_u32) !=  tmp_u32)
  {
    if((uwTick - Tickstart) > 5)
      break;
  }

  //12. SCL 및 SDA I/O를 대체 기능 Open-Drain으로 구성.
  switch(type)
  {
    default:
    case 0: //SCL: PB6, SDA: PB7
      tmp_u32 = GPIOB->CRL;    // | b31 | b30 | b29 | b28  | b27 | b26 | b25 | b24  |
      tmp_u32 &= 0x00FFFFFFUL; // | CNF7[1:0] | MODE7[1:0] | CNF6[1:0] | MODE6[1:0] |
      tmp_u32 |= 0xFF000000UL; // |  1  |  1  |  1  |  1   |  1  |  1  |  1  |  1   |
      GPIOB->CRL = tmp_u32;
      break;

    case 1: //SCL: PB8, SDA: PB9
      tmp_u32 = GPIOB->CRH;    // | b7  | b6  | b5  | b4   | b3  | b2  | b1  | b0   |
      tmp_u32 &= 0xFFFFFF00UL; // | CNF9[1:0] | MODE9[1:0] | CNF8[1:0] | MODE8[1:0] |
      tmp_u32 |= 0x000000FFUL; // |  1  |  1  |  1  |  1   |  1  |  1  |  1  |  1   |
      GPIOB->CRH = tmp_u32;
      break;

    case 2: //SCL: PB10, SDA: PB11
      tmp_u32 = GPIOB->CRH;    // | b15  | b14 | b13  | b12  | b11  | b10 |  b9  |  b8  |
      tmp_u32 &= 0xFFFF00FFUL; // | CNF11[1:0] | MODE11[1:0] | CNF10[1:0] | MODE10[1:0] |
      tmp_u32 |= 0x0000FF00UL; // |  1   |  1  |  1   |  1   |  1   |  1  |  1   |  1   |
      GPIOB->CRH = tmp_u32;
      break;
  }

  //13. I2Cx_CR1 레지스터에 SWRST 비트를 Set.
  //14. I2Cx_CR1 레지스터에 SWRST 비트를 Clear.
  //15. I2Cx_CR1 레지스터에서 PE 비트를 설정하여 I2C 주변기기를 활성화.
  switch(type)
  {
    default:
    case 0:
    case 1:
      I2C_EnableSoftwareReset(I2C1);
      I2C_DisableSoftwareReset(I2C1);
      I2C_Enable(I2C1);
      break;

    case 2:
      I2C_EnableSoftwareReset(I2C2);
      I2C_DisableSoftwareReset(I2C2);
      I2C_Enable(I2C2);
      break;
  }

  delay_us(10);
}

//STM32F1 시리즈 I2C 비지-플레그 문제 해결
void I2C_ClearBusyFlagErratum(void)
{
  if( I2C_IsEnabled(I2C1) && I2C_IsActiveFlag_BUSY(I2C1) )
  {
    if(AFIO->MAPR & 0x00000002UL)
      I2C_ClearBusyFlagHelper(1); //I2C1_REMAP=1
    else
      I2C_ClearBusyFlagHelper(0); //I2C1_REMAP=0
  }

  if( I2C_IsEnabled(I2C2) && I2C_IsActiveFlag_BUSY(I2C2) )
  {
    I2C_ClearBusyFlagHelper(2); //I2C2
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
