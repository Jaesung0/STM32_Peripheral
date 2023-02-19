 /*----------------------------------------------------------------------------
  Project :  STM32F1 I²C Busy Flag Erratum (Register)
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8

  I²C 아날로그 필터문제로 인한, Busy Flag가 High 상태로 고정되는 문제 해결

  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  ----------------------------------------------------------------------------*/
#include "main.h"
#include "i2c_ERR_F1.h"

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
      I2C1->CR1 &= 0xFFFFFFFE; //I2C_Disable

      tmp_u32 = GPIOB->CRL;    // | b31 | b30 | b29 | b28  | b27 | b26 | b25 | b24  |
      tmp_u32 &= 0x00FFFFFFUL; // | CNF7[1:0] | MODE7[1:0] | CNF6[1:0] | MODE6[1:0] |
      tmp_u32 |= 0x77000000UL; // |  0  |  1  |  1  |  1   |  0  |  1  |  1  |  1   |
      GPIOB->CRL = tmp_u32;

      tmp_u32 = 0x000000C0UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 1: //SCL: PB8, SDA: PB9
      I2C1->CR1 &= 0xFFFFFFFE; //I2C_Disable

      tmp_u32 = GPIOB->CRH;    // | b7  | b6  | b5  | b4   | b3  | b2  | b1  | b0   |
      tmp_u32 &= 0xFFFFFF00UL; // | CNF9[1:0] | MODE9[1:0] | CNF8[1:0] | MODE8[1:0] |
      tmp_u32 |= 0x00000077UL; // |  0  |  1  |  1  |  1   |  0  |  1  |  1  |  1   |
      GPIOB->CRH = tmp_u32;

      tmp_u32 = 0x00000300UL;
      GPIOB->ODR |= tmp_u32;
      break;

    case 2: //SCL: PB10, SDA: PB11
      I2C2->CR1 &= 0xFFFFFFFE; //I2C_Disable

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
      I2C1->CR1 |= 0x00008000; //Software Reset
      asm("nop");
      I2C1->CR1 &= 0xFFFF7FFF;
      asm("nop");
      I2C1->CR1 |= 0x00000001; //I2C_Enable
      asm("nop");
      break;

    case 2:
      I2C2->CR1 |= 0x00008000; //Software Reset
      asm("nop");
      I2C2->CR1 &= 0xFFFF7FFF;
      asm("nop");
      I2C2->CR1 |= 0x00000001; //I2C_Enable
      asm("nop");
      break;
  }
}

//STM32F1 시리즈 I²C Busy Flag 문제 해결
void I2C_ClearBusyFlagErratum(void)
{
  if( (I2C1->CR1 & 0x00000001) && (I2C1->SR2 & 0x00000002) ) // I2C_Enabled && BUSY_Flag
  {
    if(AFIO->MAPR & 0x00000002UL)
      I2C_ClearBusyFlagHelper(1); //I2C1_REMAP=1
    else
      I2C_ClearBusyFlagHelper(0); //I2C1_REMAP=0
  }

  if( (I2C2->CR1 & 0x00000001) && (I2C2->SR2 & 0x00000002) ) // I2C_Enabled && BUSY_Flag
  {
    I2C_ClearBusyFlagHelper(2); //I2C2
  }
}
