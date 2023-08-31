 /*----------------------------------------------------------------------------
  Project : 버튼입력
  Author  : Jaesung Oh
  TEXT Encoding : UTF-8
  ----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion */
#ifndef __BUTTON_PROC_H
#define __BUTTON_PROC_H
#ifdef __cplusplus
 extern "C" {
#endif

typedef enum //_BtnValue_t
{
  BTN_NONE = 0, //
  BTN_RELEASE,  // 떨어짐
  BTN_PUSH,     // 눌림
  BTN_LONG,     // 길게눌림
  BTN_REPEAT
}BtnValue_t;

void BTN_TIM10ms_Process(void);//타이머 인터럽트에서 10ms 단위로 실행
void Button_Loop_Process(void);//main.c 에서 반복실행

#ifdef __cplusplus
}
#endif
#endif /*__ BUTTON_PROC_H */
