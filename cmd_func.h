#ifndef __CMD_FUNC_H
#define __CMD_FUNC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmdline.h"

#define CMD_BUF_SIZE            128

/* Command function prototypes */
extern tCmdLineEntry g_psCmdTable[];
extern char g_CmdBuf[CMD_BUF_SIZE];

void DBG_RxProcess(USART_TypeDef *USARTx); //디버그용 UART 입력문자 처리
void del_CmdLine(void);//커맨드 라인 문자 자우기
void print_CmdBuf(void); //커맨트 버퍼에 있는 문자 출력

#ifdef __cplusplus
}
#endif
#endif  /* CMD_FUNC_H */
