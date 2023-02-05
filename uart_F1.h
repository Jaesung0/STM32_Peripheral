/*----------------------------------------------------------------------------
  Project : STM32 UART transmit
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __UART_TRX_H
#define __UART_TRX_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "defines.h"

//사용하는 UART 설정
#ifndef USE_USART1
 #define USE_USART1  1
#endif
#ifndef USE_USART2
 #define USE_USART2  0
#endif
#ifndef USE_USART3
 #define USE_USART3  0
#endif
#ifndef USE_USART4
 #define USE_UART4   0
#endif
#ifndef USE_UART5
 #define USE_UART5   0
#endif

#define UART_Enable(UARTX)            (UARTX)->CR1 |= 0x00000001
#define UART_Disable(UARTX)           (UARTX)->CR1 &= ~(0x00000001)
#define UART_EnableRx(UARTX)          (UARTX)->CR1 |= 0x00000004
#define UART_DisableRx(UARTX)         (UARTX)->CR1 &= ~(0x00000004)
#define UART_EnableTx(UARTX)          (UARTX)->CR1 |= 0x00000008
#define UART_DisableTx(UARTX)         (UARTX)->CR1 &= ~(0x00000008)
#define UART_EnableIT_RXNE(UARTX)     (UARTX)->CR1 |= 0x00000020
#define UART_DisableIT_RXNE(UARTX)    (UARTX)->CR1 &= ~(0x00000020)
#define UART_IsEnabledIT_RXNE(UARTX)  (UARTX)->CR1 & 0x00000020
#define UART_EnableIT_TC(UARTX)       (UARTX)->CR1 |= 0x00000040
#define UART_DisableIT_TC(UARTX)      (UARTX)->CR1 &= ~(0x00000040)
#define UART_IsEnabledIT_TC(UARTX)    (UARTX)->CR1 & 0x00000040
#define UART_EnableIT_TXE(UARTX)      (UARTX)->CR1 |= 0x00000080
#define UART_DisableIT_TXE(UARTX)     (UARTX)->CR1 &= ~(0x00000080)
#define UART_IsEnabledIT_TXE(UARTX)   (UARTX)->CR1 & 0x00000080
#define UART_IsActiveFlag_PE(UARTX)   (UARTX)->SR & 0x00000001
#define UART_IsActiveFlag_FE(UARTX)   (UARTX)->SR & 0x00000002
#define UART_IsActiveFlag_NE(UARTX)   (UARTX)->SR & 0x00000004
#define UART_IsActiveFlag_ORE(UARTX)  (UARTX)->SR & 0x00000008
#define UART_IsActiveFlag_RXNE(UARTX) (UARTX)->SR & 0x00000020
#define UART_IsActiveFlag_TC(UARTX)   (UARTX)->SR & 0x00000040
#define UART_IsActiveFlag_TXE(UARTX)  (UARTX)->SR & 0x00000080
#define UART_ClearFlag_TC(UARTX)      (UARTX)->SR &= ~(0x00000040)
#define UART_TransmitData8(UARTX)     (UARTX)->DR
#define UART_ReceiveData8(UARTX)      (UARTX)->DR

typedef struct _Queue //Queue 구조체 정의
{
  uint8_t *buf;
  volatile uint16_t size;
  volatile uint16_t front;
  volatile uint16_t rear;
  volatile uint16_t count;
}Queue;

void UART_SetBaud(USART_TypeDef *USARTx, uint32_t BaudRate); //UART 보드레이트 설정
void UART_TXchar(USART_TypeDef *USARTx, char data);   //UART로 1개 문자 전송, 폴링방식
void UART_TXstring(USART_TypeDef *USARTx, void *string); //UART로 문자열 전송, 최대 255문자, 폴링방식
void UART_TXdata(USART_TypeDef *USARTx, void *data, uint16_t len); //UART로 DATA 전송, 폴링방식
void UART_TXB_Init(USART_TypeDef *USARTx, uint16_t size); //UART 송신버퍼 생성 및 초기화
void UART_RXB_Init(USART_TypeDef *USARTx, uint16_t size); //UART 수신버퍼 생성 및 초기화
void UART_TXcharNB(USART_TypeDef *USARTx, char data); //UART로 1개 문자 전송, Non-blocking 방식
void UART_TXstringNB(USART_TypeDef *USARTx, void *string); //UART로 문자열 전송, Non-blocking 방식
void UART_TXdataNB(USART_TypeDef *USARTx, void *data, uint16_t len); //UART로 다수의 DATA 전송, Non-blocking 방식
void UART_TxEmptyCallback(USART_TypeDef *USARTx); //송신데이터 없음 인터럽트 처리
void UART_WiteTXcpltNB(USART_TypeDef *USARTx); //UART 송신버퍼가 비워질때까지 대기
void UART_RxCpltCallback(USART_TypeDef *USARTx); //수신데이터 있음 인터럽트 처리
uint8_t UART_RXB_Count(USART_TypeDef *USARTx); //UART 수신버퍼에 있는 데이터크기 확인
uint8_t UART_RXbytePop(USART_TypeDef *USARTx); //UART 수신버퍼에서 1바이트 데이터 추출(큐에서 front 삭제)
uint8_t UART_RXbytePeek(USART_TypeDef *USARTx); //UART 수신버퍼에서 1바이트 데이터 확인(큐에서 front 삭제 안함)
void UART_RXbytePush(USART_TypeDef *USARTx, uint8_t RxData); //UART 수신버퍼에 1byte 데이터 추가
void UART_RXdataPush(USART_TypeDef *USARTx, void *data, uint16_t len); //UART 수신버퍼에 다수의 데이터 추가
void UART_RXstringPush(USART_TypeDef *USARTx, void *string); //UART 수신버퍼에 문자열 추가
void UART_RXdataClear(USART_TypeDef *USARTx); //UART 수신버퍼 비우기
int UART_Printf(USART_TypeDef *USARTx, const char *format, ...); //지정된 UART로 출력, 최대 255문자

#ifdef __cplusplus
}
#endif
#endif /*__ UART_TRX_H */
