 /*----------------------------------------------------------------------------
  Project : 버튼입력
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

#include <stdio.h>
#include "main.h"
#include "defines.h"
#include "button.h"
#include "cmd_func.h"
#include "uart_F1.h"

#ifndef BTN_isIN
 #define BTN_isIN()           !LL_GPIO_IsInputPinSet(B1_GPIO_Port, B1_Pin)
#endif

#define BTN_LONG_TIME        200 //2초


static volatile BtnValue_t gBtnNow, gBtnPast = BTN_RELEASE;
static volatile uint16_t gBTN_Cnt;

//10ms주기로 실행, 버튼 입력상태 변동 확인하여 버튼상태 변수값에 저장
void BTN_TIM10ms_Process(void)
{
  if( BTN_isIN() )
  {
    if( (gBtnNow != BTN_LONG) && (gBtnPast != BTN_LONG) )
      gBTN_Cnt++;

    if(gBtnNow == BTN_RELEASE) //Button_Loop_Process 진입전에 버튼이 변경된 경우
      gBTN_Cnt = 0; //RELEASE 가 먼저 처리 되도록 gBTN_Cnt 을 0으로

    if( (gBTN_Cnt) && (gBtnPast == BTN_RELEASE) ) //버튼 눌림
    {
      gBtnNow = BTN_PUSH;
    }
    else if( (gBTN_Cnt > BTN_LONG_TIME) && (gBtnPast == BTN_PUSH) )
    {
      gBtnNow = BTN_LONG;
    }
  }
  else
  {
    if(gBtnNow == BTN_PUSH) //Button_Loop_Process 진입전에 버튼이 변경된 경우
    {
      gBtnNow = BTN_NONE; //20ms 내 눌렀다 놓인것으로 무시
      gBTN_Cnt = 0;
    }

    if( (gBtnPast > BTN_RELEASE) || (gBtnNow > BTN_RELEASE))
    {
      gBtnNow = BTN_RELEASE;
      gBTN_Cnt = 0;
    }
  }
}

//main.c 에서 반복실행
void Button_Loop_Process(void)
{
  BtnValue_t Btn = gBtnNow; //인터럽트에서 변경될 수 있는 변수는 복사하여 사용

  if(Btn)
  {
    del_CmdLine();
    printf(" Key: ");
    switch(Btn)
    {
      case BTN_PUSH:
        printf("Pressing");
        //Event_BTN_Press();
        break;

      case BTN_LONG:
        printf("Long Pressing");
        //Event_BTN_LongKey_Press();
        break;

      case BTN_RELEASE:
        if(gBtnPast == BTN_PUSH)
        {
          printf("Released(Short Key)");
          //Event_BTN_ShortKey_Release();
        }
        else//if(gBtnPast == BTN_LONG)
        {
          printf("Released(Long Key)");
          //Event_BTN_LongKey_Release();
        }
        break;

      default:
        break;
    }
    printf("\r\ncmd>");
    print_CmdBuf();

    gBtnPast = Btn;
    gBtnNow = BTN_NONE;
  }
}
