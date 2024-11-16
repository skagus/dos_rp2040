#pragma once

#include "types.h"
#include "sched.h"
#include "lld_cdc.h"

#define UART_BPS        (38400UL)
#define NOT_NUMBER      (0x7FFFFFFF)

typedef void (CmdHandler)(uint8 argc, char* argv[]);

void CLI_Init();
void CLI_Register(char* szCmd, CmdHandler pHandle);
void CLI_Printf(char* szFmt, ...);
uint32 CLI_GetInt(char* szStr);
