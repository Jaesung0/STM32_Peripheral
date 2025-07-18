 /*----------------------------------------------------------------------------
  Project : STM32F7 UART
  Author  : Jaesung Oh
            https://github.com/Jaesung0/STM32_Peripheral
  TEXT Encoding : UTF-8

  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  This software is provided AS-IS.

  -- 사용자 define 에 추가 --
#define USE_USART1          1
#define USART1_TX_BUF_SIZE  1024
#define USART1_RX_BUF_SIZE  128
#define UART_DBG            USART1 //디버그 메시지 UART

  -- main.c 에 추가 --
// [printf() 리디렉션] 
#if (defined __CC_ARM) || (defined __ARMCC_VERSION) || (defined __ICCARM__)
 int fputc(int ch, FILE *f)
#else
 int __io_putchar(int ch)
#endif
 {
   #if SWV_Trace_EN
   ITM_SendChar( (uint32_t)ch );
   #else
   UART_TXcharNB(UART_DBG, (char)ch);
   #endif

   return ch;
 }

-- int main(void) 내부에 추가 --
  setvbuf(stdout, NULL, _IONBF, 0); // 즉시 printf 가 송신될수 있도록 stdout buffer size를 0으로 설정
  //UART_SetBaud(UART_DBG, 115200);
  //HAL_Delay(1);

  //송신 활성화, 수신 활성화, USART 활성화 - MX_USARTx_UART_Init() 에서 실행
  //UART_EnableTx(USARTx);
  //UART_EnableRx(USARTx);
  //UART_Enable(USARTx);
  
  //DBG UART 수신인터럽트 동작
  UART_EnableIT_RXNE(UART_DBG);
  ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "main.h"
#include "uart_F7_StaticVar.h"

//사용자 define include
//#include "defines.h"

typedef struct _Queue //Queue 구조체 정의
{
  uint8_t *buf;
  volatile uint16_t size;
  volatile uint16_t front;
  volatile uint16_t rear;
}Queue;

//사용하는 UART 설정
#ifndef USE_USART1
 #define USE_USART1   0
#endif
#ifndef USE_USART2
 #define USE_USART2   0
#endif
#ifndef USE_USART3
 #define USE_USART3   0
#endif
#ifndef USE_UART4
 #define USE_UART4    0
#endif
#ifndef USE_UART5
 #define USE_UART5    0
#endif
#ifndef USE_USART6
 #define USE_USART6   0
#endif
#ifndef USE_UART7
 #define USE_UART7    0
#endif
#ifndef USE_UART8
 #define USE_UART8    0
#endif

#if USE_USART1
 #ifndef USART1_TX_BUF_SIZE
  #define USART1_TX_BUF_SIZE  16
 #endif
 #ifndef USART1_RX_BUF_SIZE
  #define USART1_RX_BUF_SIZE  16
 #endif
  static uint8_t U1TxBuf[USART1_TX_BUF_SIZE];
  static uint8_t U1RxBuf[USART1_RX_BUF_SIZE];
  static Queue U1TXB = {U1TxBuf, USART1_TX_BUF_SIZE, 0, 0};
  static Queue U1RXB = {U1RxBuf, USART1_RX_BUF_SIZE, 0, 0};
#endif

#if USE_USART2
 #ifndef USART2_TX_BUF_SIZE
  #define USART2_TX_BUF_SIZE  16
 #endif
 #ifndef USART2_RX_BUF_SIZE
  #define USART2_RX_BUF_SIZE  16
 #endif
  static uint8_t U2TxBuf[USART2_TX_BUF_SIZE];
  static uint8_t U2RxBuf[USART2_RX_BUF_SIZE];
  static Queue U2TXB = {U2TxBuf, USART2_TX_BUF_SIZE, 0, 0};
  static Queue U2RXB = {U2RxBuf, USART2_RX_BUF_SIZE, 0, 0};
#endif

#if USE_USART3
 #ifndef USART3_TX_BUF_SIZE
  #define USART3_TX_BUF_SIZE  16
 #endif
 #ifndef USART3_RX_BUF_SIZE
  #define USART3_RX_BUF_SIZE  16
 #endif
  static uint8_t U3TxBuf[USART3_TX_BUF_SIZE];
  static uint8_t U3RxBuf[USART3_RX_BUF_SIZE];
  static Queue U3TXB = {U3TxBuf, USART3_TX_BUF_SIZE, 0, 0};
  static Queue U3RXB = {U3RxBuf, USART3_RX_BUF_SIZE, 0, 0};
#endif

#if USE_UART4
 #ifndef UART4_TX_BUF_SIZE
  #define UART4_TX_BUF_SIZE  16
 #endif
 #ifndef UART4_RX_BUF_SIZE
  #define UART4_RX_BUF_SIZE  16
 #endif
  static uint8_t U4TxBuf[UART4_TX_BUF_SIZE];
  static uint8_t U4RxBuf[UART4_RX_BUF_SIZE];
  static Queue U4TXB = {U4TxBuf, UART4_TX_BUF_SIZE, 0, 0};
  static Queue U4RXB = {U4RxBuf, UART4_RX_BUF_SIZE, 0, 0};
#endif

#if USE_UART5
 #ifndef UART5_TX_BUF_SIZE
  #define UART5_TX_BUF_SIZE  16
 #endif
 #ifndef UART5_RX_BUF_SIZE
  #define UART5_RX_BUF_SIZE  16
 #endif
  static uint8_t U5TxBuf[UART5_TX_BUF_SIZE];
  static uint8_t U5RxBuf[UART5_RX_BUF_SIZE];
  static Queue U5TXB = {U5TxBuf, UART5_TX_BUF_SIZE, 0, 0};
  static Queue U5RXB = {U5RxBuf, UART5_RX_BUF_SIZE, 0, 0};
#endif

#if USE_USART6
 #ifndef USART6_TX_BUF_SIZE
  #define USART6_TX_BUF_SIZE  16
 #endif
 #ifndef USART6_RX_BUF_SIZE
  #define USART6_RX_BUF_SIZE  16
 #endif
  static uint8_t U6TxBuf[USART6_TX_BUF_SIZE];
  static uint8_t U6RxBuf[USART6_RX_BUF_SIZE];
  static Queue U6TXB = {U6TxBuf, USART6_TX_BUF_SIZE, 0, 0};
  static Queue U6RXB = {U6RxBuf, USART6_RX_BUF_SIZE, 0, 0};
#endif

#if USE_UART7
 #ifndef UART7_TX_BUF_SIZE
  #define UART7_TX_BUF_SIZE  16
 #endif
 #ifndef UART7_RX_BUF_SIZE
  #define UART7_RX_BUF_SIZE  16
 #endif
  static uint8_t U7TxBuf[UART7_TX_BUF_SIZE];
  static uint8_t U7RxBuf[UART7_RX_BUF_SIZE];
  static Queue U7TXB = {U7TxBuf, UART7_TX_BUF_SIZE, 0, 0};
  static Queue U7RXB = {U7RxBuf, UART7_RX_BUF_SIZE, 0, 0};
#endif

#if USE_UART8
 #ifndef UART8_TX_BUF_SIZE
  #define UART8_TX_BUF_SIZE  16
 #endif
 #ifndef UART8_RX_BUF_SIZE
  #define UART8_RX_BUF_SIZE  16
 #endif
  static uint8_t U8TxBuf[UART8_TX_BUF_SIZE];
  static uint8_t U8RxBuf[UART8_RX_BUF_SIZE];
  static Queue U8TXB = {U8TxBuf, UART8_TX_BUF_SIZE, 0, 0};
  static Queue U8RXB = {U8RxBuf, UART8_RX_BUF_SIZE, 0, 0};
#endif

//USART 보드레이트 설정
//Clock Configuration 에서 UsARTx 클럭 소스를 PCLKx 로 설정한 경우에만 유효하게 설정됨
//110,300,600,1200,2400,4800,9600,14400,19200,38400,57600,115200,230400,460800,921600
void UART_SetBaud(USART_TypeDef *USARTx, uint32_t Baud)
{
  //USART1, USART6  APB2 버스, 나머지는 APB1버스
  uint32_t PClk; // = SystemCoreClock;

 #if USE_USART1
  if(USARTx == USART1)
  {
    PClk = SystemCoreClock >> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos];
  }
  else
 #endif
 #if USE_USART6
  if(USARTx == USART6)
  {
    PClk = SystemCoreClock >> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos];
  }
  else
 #endif
  {
    PClk = SystemCoreClock >> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos];
  }

  //LL_USART_SetBaudRate(USARTx, PeriClk, BaudRate);
  USARTx->BRR = (uint16_t)( ((((((PClk*25)/(4*Baud))/100) << 4) +
  ((((((PClk*25)/(4*Baud)) - ((((PClk*25)/(4*Baud))/100) * 100)) * 16 + 50) / 100) & 0xF0)) +
  ((((((PClk*25)/(4*Baud)) - ((((PClk*25)/(4*Baud))/100) * 100)) * 16 + 50) / 100) & 0x0F)) );
}

//UART로 1개 문자 전송, polling 방식
//UART_TXchar(USART1, 'A');
void UART_TXchar(USART_TypeDef *USARTx, char data)
{
  while( !(UART_IsActiveFlag_TXE(USARTx)) );// wait until TXE = 1

  UART_TransmitData8(USARTx) = data;
}

//UART로 문자열 전송, 최대 255문자, polling 방식
//UART_TXstring(USART1, "0123456789\r\n");
void UART_TXstring(USART_TypeDef *USARTx, void *string)
{
  uint16_t i = 0;

  while(*(char*)string != '\0')
  {
    while( !(UART_IsActiveFlag_TXE(USARTx)) );

    UART_TransmitData8(USARTx) = *(char*)string++;

    if(i++ > 254)
      break;
  }
}

//UART로 다수의 DATA 전송, polling 방식
/*
 *  str_size = sprintf(str, "0123456789\r\n");
 *  UART_TXdata(USART1, str, str_size);
 */
void UART_TXdata(USART_TypeDef *USARTx, void *data, uint16_t len)
{
  uint16_t i = 0;

  for(i=0; i<len; i++)
  {
    while( !(UART_IsActiveFlag_TXE(USARTx)) );

    UART_TransmitData8(USARTx) = *(uint8_t*)data++;
  }
}

//UART로 1개 문자 전송, Non-blocking 방식
void UART_TXcharNB(USART_TypeDef *USARTx, char data)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if(U1TXB.size == 0)
        return;
      if( (U1TXB.rear+1)%U1TXB.size == U1TXB.front ) //버퍼초과 방지 
        return;
      UART_DisableIT_TXE(USARTx); //송신데이터 없음 인터럽트 비활성화
      U1TXB.buf[U1TXB.rear] = data; //버퍼에 추가
      U1TXB.rear = (U1TXB.rear+1)%U1TXB.size;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2TXB.size == 0)
        return;
      if( (U2TXB.rear+1)%U2TXB.size == U2TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U2TXB.buf[U2TXB.rear] = data;
      U2TXB.rear = (U2TXB.rear+1)%U2TXB.size;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3TXB.size == 0)
        return;
      if( (U3TXB.rear+1)%U3TXB.size == U3TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U3TXB.buf[U3TXB.rear] = data;
      U3TXB.rear = (U3TXB.rear+1)%U3TXB.size;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4TXB.size == 0)
        return;
      if( (U4TXB.rear+1)%U4TXB.size == U4TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U4TXB.buf[U4TXB.rear] = data;
      U4TXB.rear = (U4TXB.rear+1)%U4TXB.size;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5TXB.size == 0)
        return;
      if( (U5TXB.rear+1)%U5TXB.size == U5TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U5TXB.buf[U5TXB.rear] = data;
      U5TXB.rear = (U5TXB.rear+1)%U5TXB.size;
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      if(U6TXB.size == 0)
        return;
      if( (U6TXB.rear+1)%U6TXB.size == U6TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U6TXB.buf[U6TXB.rear] = data;
      U6TXB.rear = (U6TXB.rear+1)%U6TXB.size;
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      if(U7TXB.size == 0)
        return;
      if( (U7TXB.rear+1)%U7TXB.size == U7TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U7TXB.buf[U7TXB.rear] = data;
      U7TXB.rear = (U7TXB.rear+1)%U7TXB.size;
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      if(U8TXB.size == 0)
        return;
      if( (U8TXB.rear+1)%U8TXB.size == U8TXB.front )
        return;
      UART_DisableIT_TXE(USARTx);
      U8TXB.buf[U8TXB.rear] = data;
      U8TXB.rear = (U8TXB.rear+1)%U8TXB.size;
      break;
   #endif

    default:
      break;
  }

  UART_EnableIT_TXE(USARTx); //송신데이터 없음 인터럽트 활성화
}

//UART로 문자열 전송, 최대 255문자, Non-blocking 방식
//UART_TXstringNB(USART1, "0123456789\r\n");
void UART_TXstringNB(USART_TypeDef *USARTx, void *string)
{
  uint16_t i = 0;

  while(*(uint8_t*)string != '\0')
  {
    UART_TXcharNB(USARTx, *(uint8_t*)string++);

    if(i++ > 254)
      break;
  }
}

//UART로 다수의 DATA 전송, Non-blocking 방식
/*
 *  str_size = sprintf(str, "0123456789\r\n");
 *  UART_TXdataNB(USART1, str, str_size);
 */
void UART_TXdataNB(USART_TypeDef *USARTx, void *data, uint16_t len)
{
  for(uint8_t i=0; i<len; i++)
    UART_TXcharNB(USARTx, *(uint8_t*)data++);
}

//송신데이터 없음 인터럽트 처리
/* stm32f7xx_it.c 의 USARTx_IRQHandler 에 추가 할 내용
 * //Transmit Data Rgister Empty
 *  if(UART_IsActiveFlag_TXE(USARTx))
 *  {
 *    //if(UART_IsEnabledIT_TXE(USARTx))
 *      UART_TxEmptyCallback(USARTx);
 *  }
 */
void UART_TxEmptyCallback(USART_TypeDef *USARTx)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if((U1TXB.rear == U1TXB.front) || (U1TXB.size == 0)) //송신버퍼 데이터 없음
      {
        UART_DisableIT_TXE(USARTx); //송신데이터 없음 인터럽트 비활성화
        return;
      }

      UART_TransmitData8(USARTx) = U1TXB.buf[U1TXB.front];
      U1TXB.front = (U1TXB.front+1)%U1TXB.size;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if((U2TXB.rear == U2TXB.front) || (U2TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U2TXB.buf[U2TXB.front];
      U2TXB.front = (U2TXB.front+1)%U2TXB.size;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if((U3TXB.rear == U3TXB.front) || (U3TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U3TXB.buf[U3TXB.front];
      U3TXB.front = (U3TXB.front+1)%U3TXB.size;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if((U4TXB.rear == U4TXB.front) || (U4TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U4TXB.buf[U4TXB.front];
      U4TXB.front = (U4TXB.front+1)%U4TXB.size;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if((U5TXB.rear == U5TXB.front) || (U5TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U5TXB.buf[U5TXB.front];
      U5TXB.front = (U5TXB.front+1)%U5TXB.size;
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      if((U6TXB.rear == U6TXB.front) || (U6TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U6TXB.buf[U6TXB.front];
      U6TXB.front = (U6TXB.front+1)%U6TXB.size;
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      if((U7TXB.rear == U7TXB.front) || (U7TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U7TXB.buf[U7TXB.front];
      U7TXB.front = (U7TXB.front+1)%U7TXB.size;
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      if((U8TXB.rear == U8TXB.front) || (U8TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U8TXB.buf[U8TXB.front];
      U8TXB.front = (U8TXB.front+1)%U8TXB.size;
      break;
   #endif

    default:
      break;
  }
}

//UART 송신버퍼가 비워질때까지 대기
//인터럽트 활성화 여부와 인터럽트 우선순위 상황을 염두하고 사용할것
void UART_WiteTXcpltNB(USART_TypeDef *USARTx)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      while(U1TXB.rear != U1TXB.front);
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      while(U2TXB.rear != U2TXB.front);
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      while(U3TXB.rear != U3TXB.front);
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      while(U4TXB.rear != U4TXB.front);
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      while(U5TXB.rear != U5TXB.front);
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      while(U6TXB.rear != U6TXB.front);
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      while(U7TXB.rear != U7TXB.front);
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      while(U8TXB.rear != U8TXB.front);
      break;
   #endif

    default:
      break;
  }
}

//수신데이터 있음 인터럽트 처리
/* stm32f7xx_it.c 의 USARTx_IRQHandler 에 추가 할 내용
 * //Receive Data Rgister Not Empty
 * if(UART_IsActiveFlag_RXNE(USARTx))
 * {
 *   //if(UART_IsEnabledIT_RXNE(USARTx))
 *     UART_RxCpltCallback(USARTx);
 * }
 */
void UART_RxCpltCallback(USART_TypeDef *USARTx)
{
  uint8_t RxData;

  //if( UART_IsActiveFlag_NE(USARTx) ) //Noises Dectection
  UART_ClearFlag_NE(USARTx);

  //if( UART_IsActiveFlag_ORE(USARTx) ) //Overrun Error
  UART_ClearFlag_ORE(USARTx);

  //if( UART_IsActiveFlag_FE(USARTx) ) //Framing Error
  UART_ClearFlag_FE(USARTx);

  //Parity Error
  if( UART_IsActiveFlag_PE(USARTx) )
  {
    UART_ClearFlag_PE(USARTx);
    RxData = UART_ReceiveData8(USARTx);
    return;
  }

  RxData = UART_ReceiveData8(USARTx);
  UART_RXbytePush(USARTx, RxData);
}

//UART 수신버퍼에 있는 데이터크기 확인
uint8_t UART_RXB_Count(USART_TypeDef *USARTx)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      return (U1RXB.size + U1RXB.rear - U1RXB.front ) % U1RXB.size;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      return (U2RXB.size + U2RXB.rear - U2RXB.front ) % U2RXB.size;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      return (U3RXB.size + U3RXB.rear - U3RXB.front ) % U3RXB.size;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      return (U4RXB.size + U4RXB.rear - U4RXB.front ) % U4RXB.size;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      return (U5RXB.size + U5RXB.rear - U5RXB.front ) % U5RXB.size;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      return (U6RXB.size + U6RXB.rear - U6RXB.front ) % U6RXB.size;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      return (U7RXB.size + U7RXB.rear - U7RXB.front ) % U7RXB.size;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      return (U8RXB.size + U8RXB.rear - U8RXB.front ) % U8RXB.size;
   #endif

    default:
      return 0;
  }
}

//UART 수신버퍼에서 데이터 추출(큐에서 front 삭제)
uint8_t UART_RXbytePop(USART_TypeDef *USARTx)
{
  uint8_t RxData = 0;

  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if(U1RXB.rear == U1RXB.front)
        return 0; //데이터가 없는경우
       RxData = U1RXB.buf[U1RXB.front];
       U1RXB.front = (U1RXB.front+1)%U1RXB.size;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2RXB.rear == U2RXB.front)
        return 0;
      RxData = U2RXB.buf[U2RXB.front];
      U2RXB.front = (U2RXB.front+1)%U2RXB.size;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3RXB.rear == U3RXB.front)
        return 0;
      RxData = U3RXB.buf[U3RXB.front];
      U3RXB.front = (U3RXB.front+1)%U3RXB.size;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4RXB.rear == U4RXB.front)
        return 0;
      RxData = U4RXB.buf[U4RXB.front];
      U4RXB.front = (U4RXB.front+1)%U4RXB.size;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5RXB.rear == U5RXB.front)
        return 0;
      RxData = U5RXB.buf[U5RXB.front];
      U5RXB.front = (U5RXB.front+1)%U5RXB.size;
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      if(U6RXB.rear == U6RXB.front)
        return 0;
      RxData = U6RXB.buf[U6RXB.front];
      U6RXB.front = (U6RXB.front+1)%U6RXB.size;
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      if(U7RXB.rear == U7RXB.front)
        return 0;
      RxData = U7RXB.buf[U7RXB.front];
      U7RXB.front = (U7RXB.front+1)%U7RXB.size;
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      if(U8RXB.rear == U8RXB.front)
        return 0;
      RxData = U8RXB.buf[U8RXB.front];
      U8RXB.front = (U8RXB.front+1)%U8RXB.size;
      break;
   #endif

    default:
      break;
  }

  return RxData;
}

//UART 수신버퍼에서 데이터 확인(큐에서 front 삭제 안함)
uint8_t UART_RXbytePeek(USART_TypeDef *USARTx)
{
  uint8_t RxData = 0;

  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if(U1RXB.rear == U1RXB.front)
        return 0; //데이터가 없는경우
       RxData = U1RXB.buf[U1RXB.front];
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2RXB.rear == U2RXB.front)
        return 0;
      RxData = U2RXB.buf[U2RXB.front];
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3RXB.rear == U3RXB.front)
        return 0;
      RxData = U3RXB.buf[U3RXB.front];
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4RXB.rear == U4RXB.front)
        return 0;
      RxData = U4RXB.buf[U4RXB.front];
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5RXB.rear == U5RXB.front)
        return 0;
      RxData = U5RXB.buf[U5RXB.front];
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      if(U6RXB.rear == U6RXB.front)
        return 0;
      RxData = U6RXB.buf[U6RXB.front];
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      if(U7RXB.rear == U7RXB.front)
        return 0;
      RxData = U7RXB.buf[U7RXB.front];
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      if(U8RXB.rear == U8RXB.front)
        return 0;
      RxData = U8RXB.buf[U8RXB.front];
      break;
   #endif

    default:
      break;
  }

  return RxData;
}

//UART 수신버퍼에 1byte 데이터 추가
void UART_RXbytePush(USART_TypeDef *USARTx, uint8_t RxData)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if((U1RXB.buf == NULL) || ((U1RXB.rear+1)%U1RXB.size == U1RXB.front))
        return; //버퍼가 선언되지 않았거나 남은공간이 없는경우
      U1RXB.buf[U1RXB.rear] = RxData; //버퍼에 데이터 추가
      U1RXB.rear = (U1RXB.rear+1)%U1RXB.size;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if((U2RXB.buf == NULL) || ((U2RXB.rear+1)%U2RXB.size == U2RXB.front))
        return;
      U2RXB.buf[U2RXB.rear] = RxData;
      U2RXB.rear = (U2RXB.rear+1)%U2RXB.size;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if((U3RXB.buf == NULL) || ((U3RXB.rear+1)%U3RXB.size == U3RXB.front))
        return;
      U3RXB.buf[U3RXB.rear] = RxData;
      U3RXB.rear = (U3RXB.rear+1)%U3RXB.size;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if((U4RXB.buf == NULL) || ((U4RXB.rear+1)%U4RXB.size == U4RXB.front))
        return;
      U4RXB.buf[U4RXB.rear] = RxData;
      U4RXB.rear = (U4RXB.rear+1)%U4RXB.size;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if((U5RXB.buf == NULL) || ((U5RXB.rear+1)%U5RXB.size == U5RXB.front))
        return;
      U5RXB.buf[U5RXB.rear] = RxData;
      U5RXB.rear = (U5RXB.rear+1)%U5RXB.size;
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      if((U6RXB.buf == NULL) || ((U6RXB.rear+1)%U6RXB.size == U6RXB.front))
        return;
      U6RXB.buf[U6RXB.rear] = RxData;
      U6RXB.rear = (U6RXB.rear+1)%U6RXB.size;
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      if((U7RXB.buf == NULL) || ((U7RXB.rear+1)%U7RXB.size == U7RXB.front))
        return;
      U7RXB.buf[U7RXB.rear] = RxData;
      U7RXB.rear = (U7RXB.rear+1)%U7RXB.size;
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      if((U8RXB.buf == NULL) || ((U8RXB.rear+1)%U8RXB.size == U8RXB.front))
        return;
      U8RXB.buf[U8RXB.rear] = RxData;
      U8RXB.rear = (U8RXB.rear+1)%U8RXB.size;
      break;
   #endif

    default:
      break;
  }
}

//UART 수신버퍼에 다수의 데이터 추가
void UART_RXdataPush(USART_TypeDef *USARTx, void *data, uint16_t len)
{
  uint8_t RXNE_En = 0;
  
  //수신데이터 있음 인터럽트 비활성화
  if( UART_IsEnabledIT_RXNE(USARTx) )
  {
    RXNE_En = 1;
    UART_DisableIT_RXNE(USARTx);
  }

  for(uint8_t i=0; i<len; i++)
    UART_RXbytePush(USARTx, *(uint8_t*)data++);

  //수신데이터 있음 인터럽트 활성화
  if(RXNE_En)
    UART_EnableIT_RXNE(USARTx);
}

//UART 수신버퍼에 문자열 추가, 최대 255문자,
void UART_RXstringPush(USART_TypeDef *USARTx, void *string)
{
  uint16_t i = 0;
  uint8_t RXNE_En = 0;
  
  //수신데이터 있음 인터럽트 비활성화
  if( UART_IsEnabledIT_RXNE(USARTx) )
  {
    RXNE_En = 1;
    UART_DisableIT_RXNE(USARTx);
  }

  while(*(uint8_t*)string != '\0')
  {
    UART_RXbytePush(USARTx, *(uint8_t*)string++);
    
    if(i++ > 254)
      break;
  }

  //수신데이터 있음 인터럽트 활성화
  if(RXNE_En)
    UART_EnableIT_RXNE(USARTx);
}

//UART 수신버퍼 비우기
void UART_RXdataClear(USART_TypeDef *USARTx)
{
  uint8_t RXNE_En = 0;

  //수신데이터 있음 인터럽트 비활성화
  if( UART_IsEnabledIT_RXNE(USARTx) )
  {
    RXNE_En = 1;
    UART_DisableIT_RXNE(USARTx);
  }
  
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      U1RXB.front = U1RXB.rear = 0;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      U2RXB.front = U2RXB.rear = 0;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      U3RXB.front = U3RXB.rear = 0;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      U4RXB.front = U4RXB.rear = 0;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      U5RXB.front = U5RXB.rear = 0;
      break;
   #endif

   #if USE_USART6
    case (uint32_t)USART6:
      U6RXB.front = U6RXB.rear = 0;
      break;
   #endif

   #if USE_UART7
    case (uint32_t)UART7:
      U7RXB.front = U7RXB.rear = 0;
      break;
   #endif

   #if USE_UART8
    case (uint32_t)UART8:
      U8RXB.front = U8RXB.rear = 0;
      break;
   #endif
   
    default:
      break;
  }

  //수신데이터 있음 인터럽트 활성화
  if(RXNE_En)
    UART_EnableIT_RXNE(USARTx);
}

//지정된 UART로 출력, 최대 255문자
int UART_Printf(USART_TypeDef *USARTx, const char *format, ...)
{
  char buf[256] = {0,};
  int len, i;
  va_list ap;

  va_start(ap ,format);
  len = vsprintf(buf, format, ap);
  va_end(ap);

  if(len > 255)
    len = 255;

  for(i=0; i<len; i++)
    UART_TXcharNB(USARTx, buf[i]);

  return len;
}
