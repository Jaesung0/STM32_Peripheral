 /*----------------------------------------------------------------------------
  Project : STM32F1 UART
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8

  -- main.c 에 추가 --
#define UART_DBG           USART1 //디버그 메시지 UART
  
//[printf() 리디렉션] 
#ifdef __GNUC__
 //GCC
 int _write(int file, char *ptr, int len)
 {
   int index;

   for(index=0 ; index<len ; index++)
   {
     // Your target output function
     #if SWV_Trace_EN
     ITM_SendChar(*ptr++);
     #else
     UART_TXcharNB(UART_DBG, *ptr++);
     //UART_TXchar(UART_DBG, *ptr++);
     #endif
   }
   return len;
 }
#elif
 //KEIL, IAR
 int fputc(int ch, FILE *f)
 {
   #if SWV_Trace_EN
   ITM_SendChar( (uint32_t)ch );
   #else
   UART_TXcharNB(UART_DBG, (char)ch);
   //UART_TXchar(UART_DBG, (char)ch);
   #endif

   return ch;
 }
#endif

-- int main(void) 내부에 추가 --
  
  //UART_DBG 송신버퍼 생성 및 초기화
  setvbuf(stdout, NULL, _IONBF, 0); // 즉시 printf 가 송신될수 있도록 stdout buffer size를 0으로 설정
  //UART_SetBaud(UART_DBG, 115200);
  //HAL_Delay(1);
  UART_TXB_Init(UART_DBG, 1024);
  UART_RXB_Init(UART_DBG, 64);
  
  //DBG UART 수신인터럽트 동작
  UART_EnableIT_RXNE(UART_DBG);
  ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "main.h"
#include "uart_F1.h"

#if USE_USART1
  Queue U1TXB;// = {NULL, 0, 0, 0, 0};
  Queue U1RXB;
#endif

#if USE_USART2
  Queue U2TXB;
  Queue U2RXB;
#endif

#if USE_USART3
  Queue U3TXB;
  Queue U3RXB;
#endif

#if USE_UART4
  Queue U4TXB;
  Queue U4RXB;
#endif

#if USE_UART5
  Queue U5TXB;
  Queue U5RXB;
#endif

//USART 보드레이트 설정
//110,300,600,1200,2400,4800,9600,14400,19200,38400,57600,115200,230400,460800,921600
void UART_SetBaud(USART_TypeDef *USARTx, uint32_t Baud)
{
  //USART1 APB2 버스, 나머지는 APB1버스
  uint32_t PClk; // = SystemCoreClock;

 #if USE_USART1
  if(USARTx == USART1)
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

//UART 송신버퍼 생성 및 초기화
void UART_TXB_Init(USART_TypeDef *USARTx, uint16_t size)
{
  if(size < 16) //버퍼 최소크기 확보
    size = 16;

  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if(U1TXB.size) //중복할당 방지
        return;
      U1TXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U1TXB.size = size;
      U1TXB.front = U1TXB.rear = U1TXB.count = 0;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2TXB.size)
        return;
      U2TXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U2TXB.size = size;
      U2TXB.front = U2TXB.rear = U2TXB.count = 0;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3TXB.size)
        return;
      U3TXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U3TXB.size = size;
      U3TXB.front = U3TXB.rear = U3TXB.count = 0;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4TXB.size)
        return;
      U4TXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U4TXB.size = size;
      U4TXB.front = U4TXB.rear = U4TXB.count = 0;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5TXB.size)
        return;
      U5TXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U5TXB.size = size;
      U5TXB.front = U5TXB.rear = U5TXB.count = 0;
      break;
   #endif

    default:
      break;
  }

  // 송신 활성화, USART 활성화
  UART_EnableTx(USARTx);
  UART_Enable(USARTx);
}

//UART 수신버퍼 생성 및 초기화
void UART_RXB_Init(USART_TypeDef *USARTx, uint16_t size)
{
  if(size < 16) //버퍼 최소크기 확보
    size = 16;

  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if(U1RXB.size) //중복할당 방지
        return;
      U1RXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U1RXB.size = size;
      U1RXB.front = U1RXB.rear = U1RXB.count = 0;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2RXB.size)
        return;
      U2RXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U2RXB.size = size;
      U2RXB.front = U2RXB.rear = U2RXB.count = 0;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3RXB.size)
        return;
      U3RXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U3RXB.size = size;
      U3RXB.front = U3RXB.rear = U3RXB.count = 0;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4RXB.size)
        return;
      U4RXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U4RXB.size = size;
      U4RXB.front = U4RXB.rear = U4RXB.count = 0;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5RXB.size)
        return;
      U5RXB.buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
      U5RXB.size = size;
      U5RXB.front = U5RXB.rear = U5RXB.count = 0;
      break;
   #endif
 
    default:
      break;
  }

  // 수신 활성화, USART 활성화
  UART_EnableRx(USARTx);
  UART_Enable(USARTx);

  //수신데이터 있음 인터럽트 활성화 - main.c 에서 실행
  //UART_EnableIT_RXNE(USARTx);
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

      if( U1TXB.count > (U1TXB.size-2) ) //버퍼초과 방지 while(U1TXB.count == U1TXB.size);
        return;

      UART_DisableIT_TXE(USARTx); //송신데이터 없음 인터럽트 비활성화

      U1TXB.buf[U1TXB.rear] = data; //버퍼에 추가
      U1TXB.rear = (U1TXB.rear+1)%U1TXB.size;
      U1TXB.count++;
      break;
   #endif

   #if USE_USART2

    case (uint32_t)USART2:
      if(U2TXB.size == 0)
        return;

      if( U2TXB.count > (U2TXB.size-2) ) //while(U2TXB.count == U2TXB.size);
        return;

      UART_DisableIT_TXE(USARTx);

      U2TXB.buf[U2TXB.rear] = data;
      U2TXB.rear = (U2TXB.rear+1)%U2TXB.size;
      U2TXB.count++;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3TXB.size == 0)
        return;

      if( U3TXB.count > (U3TXB.size-2) ) //while(U3TXB.count == U3TXB.size);
        return;

      UART_DisableIT_TXE(USARTx);

      U3TXB.buf[U3TXB.rear] = data;
      U3TXB.rear = (U3TXB.rear+1)%U3TXB.size;
      U3TXB.count++;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4TXB.size == 0)
        return;

      if( U4TXB.count > (U4TXB.size-2) ) //while(U4TXB.count == U4TXB.size);
        return;

      UART_DisableIT_TXE(USARTx);

      U4TXB.buf[U4TXB.rear] = data;
      U4TXB.rear = (U4TXB.rear+1)%U4TXB.size;
      U4TXB.count++;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5TXB.size == 0)
        return;

      if( U5TXB.count > (U5TXB.size-2) ) //while(U5TXB.count == U5TXB.size);
        return;

      UART_DisableIT_TXE(USARTx);

      U5TXB.buf[U5TXB.rear] = data;
      U5TXB.rear = (U5TXB.rear+1)%U5TXB.size;
      U5TXB.count++;
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
/* stm32f1xx_it.c 의 USARTx_IRQHandler 에 추가 할 내용
 * //Transmit Data Rgister Empty
 *  if(UART_IsActiveFlag_TXE(USARTx))
 *  {
 *    if(UART_IsEnabledIT_TXE(USARTx))
 *      UART_TxEmptyCallback(USARTx);
 *  }
 */
void UART_TxEmptyCallback(USART_TypeDef *USARTx)
{
  switch ( (uint32_t)USARTx )
  {
   #if USE_USART1
    case (uint32_t)USART1:
      if((U1TXB.count == 0) || (U1TXB.size == 0)) //송신버퍼 데이터 없음
      {
        UART_DisableIT_TXE(USARTx); //송신데이터 없음 인터럽트 비활성화
        return;
      }

      UART_TransmitData8(USARTx) = U1TXB.buf[U1TXB.front];
      U1TXB.front = (U1TXB.front+1)%U1TXB.size;
      U1TXB.count--;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if((U2TXB.count == 0) || (U2TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U2TXB.buf[U2TXB.front];
      U2TXB.front = (U2TXB.front+1)%U2TXB.size;
      U2TXB.count--;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if((U3TXB.count == 0) || (U3TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U3TXB.buf[U3TXB.front];
      U3TXB.front = (U3TXB.front+1)%U3TXB.size;
      U3TXB.count--;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if((U4TXB.count == 0) || (U4TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U4TXB.buf[U4TXB.front];
      U4TXB.front = (U4TXB.front+1)%U4TXB.size;
      U4TXB.count--;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if((U5TXB.count == 0) || (U5TXB.size == 0))
      {
        UART_DisableIT_TXE(USARTx);
        return;
      }

      UART_TransmitData8(USARTx) = U5TXB.buf[U5TXB.front];
      U5TXB.front = (U5TXB.front+1)%U5TXB.size;
      U5TXB.count--;
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
      while(U1TXB.count);
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      while(U2TXB.count);
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      while(U3TXB.count);
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      while(U4TXB.count);
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      while(U5TXB.count);
      break;
   #endif

    default:
      break;
  }
}

//수신데이터 있음 인터럽트 처리
/* stm32f1xx_it.c 의 USARTx_IRQHandler 에 추가 할 내용
 * //Receive Data Rgister Not Empty
 * if(UART_IsActiveFlag_RXNE(USARTx))
 * {
 *   if(LL_USART_IsEnabledIT_RXNE(USARTx))
 *     UART_RxCpltCallback(USARTx);
 * }
 */
void UART_RxCpltCallback(USART_TypeDef *USARTx)
{
  uint8_t RxData;

  //Parity Error, Framing Error 
  if( UART_IsActiveFlag_PE(USARTx) || UART_IsActiveFlag_FE(USARTx) ) 
  {
    //USARTx->SR 을 읽은 후 USARTx->DR 을 읽게되면, USARTx->SR의 플레그가 초기화 됨
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
      return U1RXB.count;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      return U2RXB.count;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      return U3RXB.count;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      return U4RXB.count;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      return U5RXB.count;
   #endif

    default:
      return 0;
  }
}

//UART 수신버퍼에서 1바이트 데이터 추출(큐에서 front 삭제)
uint8_t UART_RXbytePop(USART_TypeDef *USARTx)
{
  uint8_t RxData = 0;
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
      if(U1RXB.count == 0)
        break; //데이터가 없는경우
       RxData = U1RXB.buf[U1RXB.front];
       U1RXB.front = (U1RXB.front+1)%U1RXB.size;
       U1RXB.count--;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2RXB.count == 0)
        break;
      RxData = U2RXB.buf[U2RXB.front];
      U2RXB.front = (U2RXB.front+1)%U2RXB.size;
      U2RXB.count--;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3RXB.count == 0)
        break;
      RxData = U3RXB.buf[U3RXB.front];
      U3RXB.front = (U3RXB.front+1)%U3RXB.size;
      U3RXB.count--;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4RXB.count == 0)
        break;
      RxData = U4RXB.buf[U4RXB.front];
      U4RXB.front = (U4RXB.front+1)%U4RXB.size;
      U4RXB.count--;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5RXB.count == 0)
        break;
      RxData = U5RXB.buf[U5RXB.front];
      U5RXB.front = (U5RXB.front+1)%U5RXB.size;
      U5RXB.count--;
      break;
   #endif
   
    default:
      break;
  }

  //수신데이터 있음 인터럽트 활성화
  if(RXNE_En)
    UART_EnableIT_RXNE(USARTx);

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
      if(U1RXB.count == 0)
        return 0; //데이터가 없는경우
      RxData = U1RXB.buf[U1RXB.front];
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if(U2RXB.count == 0)
        return 0;
      RxData = U2RXB.buf[U2RXB.front];
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if(U3RXB.count == 0)
        return 0;
      RxData = U3RXB.buf[U3RXB.front];
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if(U4RXB.count == 0)
        return 0;
      RxData = U4RXB.buf[U4RXB.front];
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if(U5RXB.count == 0)
        return 0;
      RxData = U5RXB.buf[U5RXB.front];
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
      if((U1RXB.buf == NULL) || (U1RXB.count >= U1RXB.size))
        return; //버퍼가 선언되지 않았거나 남은공간이 없는경우
      U1RXB.buf[U1RXB.rear] = RxData; //버퍼에 데이터 추가
      U1RXB.rear = (U1RXB.rear+1)%U1RXB.size;
      U1RXB.count++;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      if((U2RXB.buf == NULL) || (U2RXB.count >= U2RXB.size))
        return;
      U2RXB.buf[U2RXB.rear] = RxData;
      U2RXB.rear = (U2RXB.rear+1)%U2RXB.size;
      U2RXB.count++;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      if((U3RXB.buf == NULL) || (U3RXB.count >= U3RXB.size))
        return;
      U3RXB.buf[U3RXB.rear] = RxData;
      U3RXB.rear = (U3RXB.rear+1)%U3RXB.size;
      U3RXB.count++;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      if((U4RXB.buf == NULL) || (U4RXB.count >= U4RXB.size))
        return;
      U4RXB.buf[U4RXB.rear] = RxData;
      U4RXB.rear = (U4RXB.rear+1)%U4RXB.size;
      U4RXB.count++;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      if((U5RXB.buf == NULL) || (U5RXB.count >= U5RXB.size))
        return;
      U5RXB.buf[U5RXB.rear] = RxData;
      U5RXB.rear = (U5RXB.rear+1)%U5RXB.size;
      U5RXB.count++;
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
      U1RXB.front = U1RXB.rear = U1RXB.count = 0;
      break;
   #endif

   #if USE_USART2
    case (uint32_t)USART2:
      U2RXB.front = U2RXB.rear = U2RXB.count = 0;
      break;
   #endif

   #if USE_USART3
    case (uint32_t)USART3:
      U3RXB.front = U3RXB.rear = U3RXB.count = 0;
      break;
   #endif

   #if USE_UART4
    case (uint32_t)UART4:
      U4RXB.front = U4RXB.rear = U4RXB.count = 0;
      break;
   #endif

   #if USE_UART5
    case (uint32_t)UART5:
      U5RXB.front = U5RXB.rear = U5RXB.count = 0;
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
