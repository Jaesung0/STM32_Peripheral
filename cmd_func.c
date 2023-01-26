#include <stdio.h>                      /* Standard I/O .h-file               */
#include <ctype.h>                      /* Character functions                */
#include <string.h>                     /* String and memory functions        */
#include <stdlib.h>

#include "main.h"
#include "defines.h"
#include "cmdline.h"
#include "cmd_func.h"
#include "uart_F1.h" //UART_RXdataPop()

/* [추가 인클루드] */


/* [추가 변수] */


//*****************************************************************************
//
// The buffer that holds the command line.
//
//*****************************************************************************
char g_CmdBuf[CMD_BUF_SIZE];
uint16_t g_CmdRxCnt = 0;

/* Command function prototypes */
int cmd_help (int argc, char *argv[]);

/* [추가 함수 프로토타입] */
int cmd_SysReset(int argc, char *argv[]);


//*****************************************************************************
//
// This is the table that holds the command names, implementing functions, and
// brief description.
//
//*****************************************************************************
tCmdLineEntry g_psCmdTable[] =
{
    { "help",    cmd_help,         "Display list of commands" },
    { "?",       cmd_help,         "alias for help" },

    { "reset",   cmd_SysReset,     "Restart system"},
    { 0, 0, 0 }
};

unsigned long atoul(char *str)
{
  unsigned long ret = 0;
  int multi;
  char c;

  if (!strncmp(str, "0X", 2) || !strncmp(str, "0x", 2))
  {
    str += 2;	// skip "0x"
    multi = 16;
  }
  else
    multi = 10;

  c = *str++;

  while (c)
  {
    if (c >= '0' && c <= '9')
      ret = ret * multi + c - '0';
    else if (c >= 'a' && c <= 'f')
      ret = ret * multi + c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      ret = ret * multi + c - 'A' + 10;
    else
      return ret;

    c = *str++;
  }
  return ret;
}

/*-----------------------------------------------------------------------------
 *        Display Command Syntax help
 *----------------------------------------------------------------------------*/
//*****************************************************************************
//
// This function implements the "help" command.  It prints a simple list of the
// available commands with a brief description.
//
//*****************************************************************************
int cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *psEntry;

    // Print some header text.
    printf("\n\r\n\r");
    printf("Available commands\n\r");
    printf("------------------\n\r");

    // Point at the beginning of the command table.
    psEntry = &g_psCmdTable[0];

    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    while(psEntry->pcCmd)
    {
        // Print the command name and the brief description.
        printf(" %-8s: %s\n\r", psEntry->pcCmd, psEntry->pcHelp);

        // Advance to the next entry in the table.
        psEntry++;
    }

    // Return success.
    return(0);
}

//디버그용 UART 입력문자 처리
void DBG_RxProcess(USART_TypeDef *USARTx)
{
  static uint8_t DBG_WasCR = 0;

  uint8_t DBG_RxData;

  DBG_RxData = UART_RXdataPop(USARTx);

  //See if the backspace key was pressed.
  if((DBG_RxData == '\b') || (DBG_RxData == 0x7F))
  { //If there are any characters already in the buffer, then delete the last.
    if(g_CmdRxCnt)
    {
      printf("\b \b"); // Rub out the previous character.
      g_CmdRxCnt--;   // Decrement the number of characters in the buffer.
    }
  }
  //escape character was received.
  else if(DBG_RxData == 0x1B)
  {
    while(g_CmdRxCnt)
    {
      printf("\b \b"); // Rub out the previous character.
      g_CmdRxCnt--;    // Decrement the number of characters in the buffer.
    }
  }
  //If this character is LF and last was CR, then just gobble up the
  //character because the EOL processing was taken care of with the CR.
  else if((DBG_RxData == '\n') && DBG_WasCR)
  {
    DBG_WasCR = 0;
  }
  //See if a newline was received.
  else if((DBG_RxData == '\r') || (DBG_RxData == '\n'))
  { // If the character is a CR, then it may be followed by a LF which
    // should be paired with the CR.  So remember that a CR was received.
    if(DBG_RxData == '\r')
    {
      DBG_WasCR = 1;
    }
    //printf("\r\n");
    g_CmdBuf[g_CmdRxCnt] = 0;
    g_CmdRxCnt = 0;

    switch( CmdLineProcess(g_CmdBuf) )
    {
      case CMDLINE_BAD_CMD: // Handle the case of bad command.
        printf("\r\nUnknown or Bad command!");
        break;

      case CMDLINE_TOO_MANY_ARGS: // Handle the case of too many arguments.
        printf("\r\nToo many arguments for command processor!");
        break;

      case CMDLINE_NO_CMD:
      case CMDLINE_SUCCESS:
        break;

      default: // Otherwise the command was executed.  Print the error code if one was returned.
        printf("\r\nCommand returned error code!");
        break;
    }
    printf("\r\ncmd>");
  }
  //Process the received character as long as we are not at the end of the buffer.
  //If the end of the buffer has been reached then all additional characters are ignored until a newline is received.
  else if((g_CmdRxCnt < (CMD_BUF_SIZE-1)) && (DBG_RxData > 0x1F) && (DBG_RxData < 0x7F) )
  {
    g_CmdBuf[g_CmdRxCnt] = DBG_RxData; //Store the character in the caller supplied buffer.
    g_CmdRxCnt++; //Increment the count of characters received.
    printf("%c",DBG_RxData); //Reflect the character back to the user.
  }
}

//커맨드 라인 문자 자우기
void del_CmdLine(void)
{
  //for(uint16_t i=0; i<g_CmdRxCnt+4; i++)
  //  printf("\b \b");
  printf("%c[2K\r", 27); //VT100 erase entire line + Carriage Return
}

//커맨트 버퍼에 있는 문자 출력
void print_CmdBuf(void)
{
  for(uint8_t i=0; i<g_CmdRxCnt; i++)
    printf("%c",g_CmdBuf[i]);
}

/* [추가 함수] */
int cmd_SysReset(int argc, char *argv[])
{
  printf("\r\n");

  if(argc != 2)
  {
    printf(" Argument format is wrong\r\n input as\r\n");
    printf("  reset yes");
    return 0;
  }

  if( !strcmp(argv[1], "yes") || !strcmp(argv[1], "Yes") || !strcmp(argv[1], "YES") )
  {
    UART_TXstring(DBG_UART, "\r\n The system will restart...\r\n");
    NVIC_SystemReset();
  }
  else
  {
    printf(" Argument format is wrong\r\n input as\r\n");
    printf("  reset yes");
  }
  return 0;
}