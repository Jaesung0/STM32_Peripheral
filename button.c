 /*----------------------------------------------------------------------------
  Project : 버튼입력
  Author  : Jaesung Oh
            https://github.com/Jaesung0/STM32_Peripheral
  TEXT Encoding : UTF-8
  
  Attention
  This software component is licensed under the BSD 3-Clause License.
  You may not use this file except in compliance with the License. 
  You may obtain a copy of the License at: opensource.org/licenses/BSD-3-Clause
  This software is provided AS-IS.
  ----------------------------------------------------------------------------*/

#include <stdio.h>
#include "main.h"
#include "button.h"

//사용자 include
//#include "defines.h"

//사용자 define
#ifndef WKUP_isIN
 #define WKUP_isIN()        LL_GPIO_IsInputPinSet(WKUP_GPIO_Port, WKUP_Pin)
#endif
#ifndef KEY0_isIN
 #define KEY0_isIN()        !LL_GPIO_IsInputPinSet(KEY0_GPIO_Port, KEY0_Pin)
#endif
#ifndef KEY1_isIN
 #define KEY1_isIN()        !LL_GPIO_IsInputPinSet(KEY1_GPIO_Port, KEY1_Pin)
#endif

#ifndef NUM_OF_KEY
 #define NUM_OF_KEY          3
#endif

#define KEY_LONG_TIME       100 //1초

typedef void (*pfunc)(); //함수 포인터 별칭 정의

typedef struct
{
  uint8_t  FuncEnable;
  pfunc    pFuncName;
}KeyEventTableElement_t;

static volatile uint8_t    KeyINIT;
static volatile uint8_t    KeyisIn[NUM_OF_KEY];
static volatile KeyValue_t KeyNow[NUM_OF_KEY], KeyPast[NUM_OF_KEY];
static volatile uint16_t   KeyCnt[NUM_OF_KEY];

//KEY 에 할당된 'GPIO Read Pin 함수' 리스트를 작성하십시오.
// LL 드라이버는 'inline function' 으로 구성되어 있기 때문에 '함수 포인터 배열'을 사용할 수 없습니다.
static void KEY_Table_GPIOpin(void)
{
  KeyisIn[0] = (uint8_t)WKUP_isIN();
  KeyisIn[1] = (uint8_t)KEY0_isIN();
  KeyisIn[2] = (uint8_t)KEY1_isIN();
}

//KEY '이름' 리스트를 작성하십시오.
const char *KeyName[NUM_OF_KEY] =
{
  "Key WKUP",
  "Key 0",
  "Key 1"
};

//(반환값과 매개변수가 없는)Event 함수의 프로토타입을 작성하십시오
static void Event_KeyWKUP_Press(void);
static void Event_Key0_Press(void);
static void Event_Key1_Press(void);

//Event_Key_Press 함수 리스트를 작성하십시오.
static KeyEventTableElement_t EventTable_Key_Press[NUM_OF_KEY] =
{
  //실행여부, (반환값과 매개변수가 없는) 함수이름
  {1,         Event_KeyWKUP_Press},
  {1,         Event_Key0_Press},
  {1,         Event_Key1_Press}
};

//Event_LongKey_Press 함수 리스트를 작성하십시오.
static KeyEventTableElement_t EventTable_LongKey_Press[NUM_OF_KEY] =
{
  //실행여부, (반환값과 매개변수가 없는) 함수이름
  {0,         NULL},
  {0,         NULL},
  {0,         NULL}
};

//Event_ShortKey_Release 함수 리스트를 작성하십시오.
static KeyEventTableElement_t EventTable_ShortKey_Release[NUM_OF_KEY] =
{
  //실행여부, (반환값과 매개변수가 없는) 함수이름
  {0,         NULL},
  {0,         NULL},
  {0,         NULL}
};

//Event_LongKey_Release 이벤트 함수 리스트를 작성하십시오.
static KeyEventTableElement_t EventTable_LongKey_Release[NUM_OF_KEY] =
{
  //실행여부, (반환값과 매개변수가 없는) 함수이름
  {0,         NULL},
  {0,         NULL},
  {0,         NULL}
};

//(반환값과 매개변수가 없는)Event 함수를 작성하십시오
static void Event_KeyWKUP_Press(void)
{

}

static void Event_Key0_Press(void)
{

}
static void Event_Key1_Press(void)
{

}

//10ms주기로 실행, KEY 입력상태 변동 확인하여 이벤트 수행
void KEY_TIM10ms_Process(void)
{
  if(!KeyINIT)
  {
    for(uint8_t i=0; i<NUM_OF_KEY; i++)
    {
      KeyNow[i] = KEY_NONE;
      KeyPast[i] = KEY_RELEASE;
      KeyCnt[i] = 0;
    }

    KeyINIT = 1;
  }
  
  //GPIO Pin 상태를 확인
  KEY_Table_GPIOpin();
  
  //KEY 변동 확인
  for(uint8_t i=0; i<NUM_OF_KEY; i++)
  {
    if( KeyisIn[i] )
    {
      if( (KeyNow[i] != KEY_LONG) && (KeyPast[i] != KEY_LONG) )
      {
        KeyCnt[i]++;
      }

      if(KeyNow[i] == KEY_RELEASE) //Button_Loop_Process 진입전에 버튼이 변경된 경우
      {
        KeyCnt[i] = 0; //RELEASE 가 먼저 처리 되도록 KeyCnt[i] 을 0으로
      }

      if( (KeyCnt[i]) && (KeyPast[i] == KEY_RELEASE) ) //버튼 눌림
      {
        KeyNow[i] = KEY_PUSH;
      }
      else if( (KeyCnt[i] > KEY_LONG_TIME) && (KeyPast[i] == KEY_PUSH) )
      {
        KeyNow[i] = KEY_LONG;
      }
    }
    else
    {
      if(KeyNow[i] == KEY_PUSH) //Button_Loop_Process 진입전에 버튼이 변경된 경우
      {
        KeyNow[i] = KEY_NONE; //20ms 내 눌렀다 놓인것으로 무시
        KeyCnt[i] = 0;
      }

      if( (KeyPast[i] > KEY_RELEASE) || (KeyNow[i] > KEY_RELEASE))
      {
        KeyNow[i] = KEY_RELEASE;
        KeyCnt[i] = 0;
      }
    }
  }

  //이벤트 수행
  for(uint8_t i=0; i<NUM_OF_KEY; i++)
  {
    if(KeyNow[i])
    {
      printf(" %s: ", KeyName[i]);
      
      switch(KeyNow[i])
      {
        case KEY_PUSH:
          printf("Pressing");
        
          if(EventTable_Key_Press[i].FuncEnable)
          {
            EventTable_Key_Press[i].pFuncName();
          }
          break;

        case KEY_LONG:
          printf("Long Pressing");
        
          if(EventTable_LongKey_Press[i].FuncEnable)
          {
            EventTable_LongKey_Press[i].pFuncName();
          }
          break;

        case KEY_RELEASE:
          if(KeyPast[i] == KEY_PUSH)
          {
            printf("Released(Short Key)");

            if(EventTable_ShortKey_Release[i].FuncEnable)
            {
              EventTable_ShortKey_Release[i].pFuncName();
            }
          }
          else//if(KeyPast[i] == KEY_LONG)
          {
            printf("Released(Long Key)");

            if(EventTable_LongKey_Release[i].FuncEnable)
            {
              EventTable_LongKey_Release[i].pFuncName();
            }
          }
          break;

        default:
          break;
      }
      printf("\r\n");

      KeyPast[i] = KeyNow[i];
      KeyNow[i] = KEY_NONE;
    }
  }
}
