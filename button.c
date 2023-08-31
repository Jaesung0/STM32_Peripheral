 /*----------------------------------------------------------------------------
  Project : 버튼입력
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License. 
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  ----------------------------------------------------------------------------*/

#include <stdio.h>
#include "main.h"
#include "button.h"
#include "uart_F1.h"
//#include "defines.h"
//#include "cmd_func.h"

#ifndef BTN_isIN
 #define BTN_isIN()           !LL_GPIO_IsInputPinSet(B1_GPIO_Port, B1_Pin)
#endif

//버튼 재입력 지연시간 (10ms단위)
#define BTN_DelayTime     100 //1.0초
//버튼 재입력 반복시간 (10ms단위)
#define BTN_RepeatTime     50 //0.5초

static volatile BtnValue_t BtnNow, BtnPast = BTN_RELEASE;
static volatile uint16_t BTN_Cnt;
static uint8_t BTN_Long;

//10ms주기로 실행, 버튼 입력상태 변동 확인하여 버튼상태 변수값에 저장
void BTN_TIM10ms_Process(void)
{
  if( BTN_isIN() )
  {
    BTN_Cnt++;

    if(BtnNow == BTN_RELEASE) //Button_Loop_Process 진입전에 버튼이 변경된 경우
      BTN_Cnt = 0; //RELEASE 가 먼저 처리 되도록 BTN_Cnt 을 0으로

    if( (BTN_Cnt) && (BtnPast == BTN_RELEASE) ) //버튼 눌림
    {
      BtnNow = BTN_PUSH;
    }
    else if( (BTN_Cnt > BTN_DelayTime) && (BtnPast == BTN_PUSH) ) //반복 지연시간 경과
    {
      BtnNow = BTN_LONG;
    }
    else if( (BTN_Cnt > (BTN_DelayTime + BTN_RepeatTime)) && (BtnNow == BTN_NONE) ) //반복지연 + 반복주기시간 경과
    {
      BtnNow = BTN_REPEAT;
      //BTN_Cnt = BTN_DelayTime; // 반복지연 직후로 변경 => Button_Loop_Process() 에서 처리
    }

  }
  else
  {
    if(BtnNow == BTN_PUSH) //Button_Loop_Process 진입전에 버튼이 변경된 경우
    {
      BtnNow = BTN_NONE; //20ms 내 눌렀다 놓인것으로 무시
      BTN_Cnt = 0;
    }

    if( (BtnPast > BTN_RELEASE) || (BtnNow > BTN_RELEASE))
    {
      BtnNow = BTN_RELEASE;
      BTN_Cnt = 0;
    }
  }
}

//main.c 에서 반복실행
void Button_Loop_Process(void)
{
  BtnValue_t Btn = BtnNow; //인터럽트에서 변경될 수 있는 변수는 복사하여 사용

  if(Btn)
  {
    //del_CmdLine();
    printf(" Key: ");
    switch(Btn)
    {
      case BTN_PUSH:
        printf("Pressing");
        //Event_BTN_Press();
        break;

      case BTN_LONG:
        BTN_Long = 1;
        printf("Long Pressing");
        //Event_BTN_LongKey_Press();
        break;

      case BTN_REPEAT:
        BTN_Cnt = BTN_DelayTime;
        printf("Long Repeat");
        //Event_BTN_Repeat();
        break;

      case BTN_RELEASE:
        if(BTN_Long)
        {
          BTN_Long = 0;
          printf("Released(Long Key)");
          //Event_BTN_LongKey_Release();
        }
        else
        {
          printf("Released(Short Key)");
          //Event_BTN_ShortKey_Release();
        }
        break;

      default:
        break;
    }
    //printf("\r\ncmd>");
    //print_CmdBuf();

    BtnPast = Btn;
    BtnNow = BTN_NONE;
  }
}
