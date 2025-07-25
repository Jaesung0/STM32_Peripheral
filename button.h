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

typedef enum
{
  KEY_NONE = 0, //
  KEY_RELEASE,  // 떨어짐
  KEY_PUSH,     // 눌림
  KEY_LONG,     // 길게눌림
}KeyValue_t;

void KEY_TIM10ms_Process(void); //10ms 단위로 실행

#ifdef __cplusplus
}
#endif
#endif /*__ BUTTON_PROC_H */
