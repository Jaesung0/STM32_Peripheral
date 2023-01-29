/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************

  Project :
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8

  Compiler  : STM32CubeIDE 1.11.0 / STM32CubeF1 Firmware Package V1.8.4
  MCU type  : STM32F103RB (NUCLEO-F103RB)
  Crystal   : Ext Clock 8MHz
  CoreClock : 72Mhz

  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* [사용자 인클루드] */
#include <stdio.h>
#include "defines.h"
#include "tim_F1.h"
#include "uart_F1.h"
#include "cmd_func.h"
#include "button.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* [전역 변수] */
uint32_t UID[3]; //Unique device ID
uint32_t UID32;

volatile uint32_t gSYS_Count;  //카운터
volatile uint8_t gFL_SYS_Init; //초기화 상태 플레그
volatile uint8_t gFL_10ms, gFL_50ms, gFL_100ms, gFL_500ms, gFL_1s; //반복 플레그

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* [printf() 리디렉션] */
#ifdef __GNUC__
 //GCC
 int _write(int file, char *ptr, int len)
 {
  /* Implement your write code here, this is used by puts and printf */
  int index;

  for(index=0 ; index<len ; index++)
  {
    // Your target output function
    #if SWV_Trace_EN
    ITM_SendChar(*ptr++);
    #else
    UART_TXcharNB(DBG_UART, *(ptr+index));
    //UART_TXchar(DBG_UART, *(ptr+index));
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
  UART_TXcharNB(DBG_UART, (char)ch);
  //UART_TXchar(DBG_UART, (char)ch);
  #endif

  return ch;
 }
#endif

/* [인터럽트 콜백함수] */

void TIM_PeriodElapsedCallback(TIM_TypeDef *TIMx)
{
  switch ( (uint32_t)TIMx )
  {
    case (uint32_t)BASE_TIM: // 1ms 주기
      {
        gSYS_Count++;


        if(gFL_SYS_Init)
        {
          //1ms
          {

          }
          //10ms
          if(( (gSYS_Count + 1) % 10)==0)
          {
            gFL_10ms = 1;
          }
          //50ms
          if(( (gSYS_Count + 3) % 50)==0)
          {
            gFL_50ms = 1;
          }
          //100ms
          if(( (gSYS_Count + 5) % 100)==0)
          {
            gFL_100ms = 1;
          }
          //500ms
          if(( (gSYS_Count + 7) % 500)==0)
          {
            gFL_500ms = 1;
          }
          //1초
          if(( (gSYS_Count + 9) % 1000)==0)
          {
            gFL_1s = 1;
          }
        }
      }
      break;

    default:
      break;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* [셋업] */

  //Unique device ID 읽어오기
  UID[0] = *(volatile uint32_t *)UID_BASE;
  UID[1] = *(volatile uint32_t *)(UID_BASE+4);
  UID[2] = *(volatile uint32_t *)(UID_BASE+8);

  //96Bits 크기의 UID를 32Bit 크기로 축약
  {
    //[6자리 숫자] + [1자리 알파벳] 형식
    if( (UID[2] & 0xFF000000) > 0x20000000 )
    {
      UID32  =  (UID[1] >> 16) & 0x0000000FUL;
      UID32 += ((UID[1] >> 24) & 0x0000000FUL) * 10;
      UID32 +=  (UID[2]        & 0x0000000FUL) * 100;
      UID32 *= ((UID[2] >>  8) & 0x00000007UL) + 1;//31Bit 크기시 ((UID[2] >>  8) & 0x00000003UL) + 1;
    }
    else //[2자리 숫자] + [4자리 숫자 와 알파벳] 형식
    {
      uint8_t tmp[3] = {0,};

      tmp[0] = (UID[1] >> 16) & 0x7F;
      tmp[1] = (UID[1] >> 24) & 0x7F;
      tmp[2] =  UID[2] & 0x7F;

      for(uint8_t i = 0; i < 3; i++)
      {
        if(tmp[i] > 0x40) //알파벳 소문자
          tmp[i] -= 0x57;
        else if(tmp[i] > 0x40) //알파벳 대문자
          tmp[i] -= 0x37;
        else if(tmp[i] > 0x2F) //숫자
          tmp[i] -= 0x30;
      }

      UID32  = tmp[0];
      UID32 += tmp[1] * 36;
      UID32 *= tmp[2] % 6 + 1; //31Bit 크기시 tmp[2] % 3 + 1;
    }

    //Wafer num (5bits)
    UID32  = UID32 << 19;
    UID32 |= (UID[1] << 14) & 0x0007C000UL;

    //X and Y coordinates (14bits)
    UID32 |= (UID[0] >>  9) & 0x00003F80UL;
    UID32 |=  UID[0] & 0x0000007FUL;
  }

  //GPIO 초기화
  LED_ON();

  //반복타이머 동작
  BASE_TIM_Enable(BASE_TIM, BASE_TIM_FREQ);


  //UART 송신버퍼 생성 및 초기화
  {
    setvbuf(stdout, NULL, _IONBF, 0); // 즉시 printf 가 송신될수 있도록 stdout buffer size를 0으로 설정

    //UART_SetBaud(DBG_UART, 115200);
    //HAL_Delay(1);
    UART_TXB_Init(DBG_UART, 1024);
    UART_RXB_Init(DBG_UART, 128);
  }

  //펌웨어 보호설정 자동변경
 #ifdef AUTO_RDP_L1
  #if AUTO_RDP_L1
  {
    FLASH_OBProgramInitTypeDef OBInit;

    HAL_FLASHEx_OBGetConfig(&OBInit);

    if(OBInit.RDPLevel == OB_RDP_LEVEL_0)
    {
      HAL_FLASH_Unlock();
      HAL_FLASH_OB_Unlock();

      OBInit.OptionType = OPTIONBYTE_RDP;
      OBInit.RDPLevel = (uint8_t)0xBB;

      UART_TXchar(DBG_UART,27);
      UART_TXstring(DBG_UART,"[2J");//VT100 clear screen
      UART_TXchar(DBG_UART,27);
      UART_TXstring(DBG_UART,"[H");//VT100 cursor home

      UART_TXstring(DBG_UART, "\r\n Set to read protection level 1\r\n");
      UART_TXstring(DBG_UART, " If the system freezes, turn the power off and then on.\r\n");

      HAL_FLASHEx_OBProgram(&OBInit);
      HAL_FLASH_OB_Launch();
      HAL_FLASH_OB_Lock();
      HAL_FLASH_Lock();
    }
  }
  #endif
 #endif


  //Debugging message
  {
    printf("%c[2J",27); //VT100 clear screen
    printf("%c[1;1H", 27);
    printf("\r\nUnique device ID: 0x %08lX %08lX %08lX\r\n",UID[2],UID[1],UID[0]);
    printf("HCLK:%lu PCLK2:%lu PCLK1:%lu \r\n",HAL_RCC_GetHCLKFreq(), HAL_RCC_GetPCLK2Freq(), HAL_RCC_GetPCLK1Freq());
    printf("Compile Data & Time: %s, %s\r\n", __DATE__, __TIME__);
  }

  //Debugging message
  {
    printf("\r\nProgram Start...\r\n");
    UART_WiteTXcpltNB(DBG_UART);
    printf("cmd>");
  }

  //초기화 완료 플레그 설정
  gFL_SYS_Init = 1;

  //DBG UART 수신인터럽트 동작
  LL_USART_EnableIT_RXNE(DBG_UART);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* [루프] */

    if(gFL_10ms)
    {
      gFL_10ms = 0;
      BTN_TIM10ms_Process();
    }

    if(gFL_50ms)
    {
      gFL_50ms = 0;

    }

    if(gFL_100ms)
    {
      gFL_100ms = 0;

    }

    if(gFL_500ms)
    {
      gFL_500ms = 0;
      LED_TOGGLE();
    }

    if(gFL_1s)
    {
      gFL_1s = 0;

    }

    Button_Loop_Process();

    while(UART_RXB_Count(DBG_UART) != 0)
      DBG_RxProcess(DBG_UART);

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
