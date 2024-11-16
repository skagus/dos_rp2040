#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "macro.h"
#include "sched.h"
#include "cli.h"
//////////////////////////////////
#define LEN_LINE            (64)
#define MAX_CMD_COUNT       (8)
#define MAX_ARG_TOKEN       (8)

#define COUNT_LINE_BUF       (3)

/**
 * history.
*/
char gaHistBuf[COUNT_LINE_BUF][LEN_LINE];
uint8 gnPrvHist;
uint8 gnHistRef;

typedef struct _CmdInfo
{
	char* szCmd;
	CmdHandler* pHandle;
} CmdInfo;

CmdInfo gaCmds[MAX_CMD_COUNT];
uint8 gnCmds;

void cli_RunCmd(char* szCmdLine);

void cli_CmdHelp(uint8 argc, char* argv[])
{
	if (argc > 1)
	{
		uint32 nNum = CLI_GetInt(argv[1]);
		CLI_Printf("help with %08lx\r\n", nNum);
		char* aCh = (char*)&nNum;
		CLI_Printf("help with %02X %02X %02X %02X\r\n", aCh[0], aCh[1], aCh[2], aCh[3]);
	}
	else
	{
		for (uint8 nIdx = 0; nIdx < gnCmds; nIdx++)
		{
			CLI_Printf("%d: %s\r\n", nIdx, gaCmds[nIdx].szCmd);
		}
	}
}

void cli_CmdHistory(uint8 argc, char* argv[])
{
	if (argc > 1) // Run the indexed command.
	{
		uint8 nSlot = CLI_GetInt(argv[1]);
		if (nSlot < COUNT_LINE_BUF)
		{
			if (strlen(gaHistBuf[nSlot]) > 0)
			{
				cli_RunCmd(gaHistBuf[nSlot]);
			}
		}
	}
	else // 
	{
		uint8 nIdx = gnPrvHist;
		CLI_Printf("\r\n");
		do
		{
			nIdx = (nIdx + 1) % COUNT_LINE_BUF;
			if (strlen(gaHistBuf[nIdx]) > 0)
			{
				CLI_Printf("%2d> %s\r\n", nIdx, gaHistBuf[nIdx]);
			}
		} while (nIdx != gnPrvHist);
	}
}

void cbf_RxUart(uint8 tag, uint8 result)
{
	Sched_TrigAsyncEvt(BIT(EVT_UART));
}

void lb_NewEntry(char* szCmdLine)
{
	if (0 != strcmp(gaHistBuf[gnPrvHist], szCmdLine))
	{
		gnPrvHist = (gnPrvHist + 1) % COUNT_LINE_BUF;
		strcpy(gaHistBuf[gnPrvHist], szCmdLine);
	}
	gnHistRef = (gnPrvHist + 1) % COUNT_LINE_BUF;
}

uint8 lb_GetNextEntry(uint8 bInc, char* szCmdLine)
{
	uint8 nStartRef = gnHistRef;
	szCmdLine[0] = 0;
	int nGab = (true == bInc) ? 1 : -1;
	do
	{
		gnHistRef = (gnHistRef + COUNT_LINE_BUF + nGab) % COUNT_LINE_BUF;
		if (strlen(gaHistBuf[gnHistRef]) > 0)
		{
			strcpy(szCmdLine, gaHistBuf[gnHistRef]);
			return strlen(szCmdLine);
		}
	} while (nStartRef != gnHistRef);
	return 0;
}

void cli_RunCmd(char* szCmdLine)
{
	char* aTok[MAX_ARG_TOKEN];
	UART_SetCbf(NULL, NULL);
	uint8 nCnt = 0;
	char* pTok = strtok(szCmdLine, " ");
	while (NULL != pTok)
	{
		aTok[nCnt++] = pTok;
		pTok = strtok(NULL, " ");
	}
	uint8 bExecute = false;
	if (nCnt > 0)
	{
		for (uint8 nIdx = 0; nIdx < gnCmds; nIdx++)
		{
			if (0 == strcmp(aTok[0], gaCmds[nIdx].szCmd))
			{
				gaCmds[nIdx].pHandle(nCnt, aTok);
				bExecute = true;
				break;
			}
		}
	}
	if (false == bExecute)
	{
		CLI_Printf("Unknown command: %s\r\n", szCmdLine);
	}
	UART_SetCbf(cbf_RxUart, NULL);
}

void cli_Run(Evts bmEvt)
{
	static uint8 nLen = 0;
	static char aLine[LEN_LINE];
	static char aTmpLine[LEN_LINE];
	char nCh;

	if (UART_RxD(&nCh))
	{
		if (' ' <= nCh && nCh <= '~')
		{
			UART_TxD(nCh);
			aLine[nLen] = nCh;
			nLen++;
		}
		else if (('\n' == nCh) || ('\r' == nCh))
		{
			if (nLen > 0)
			{
				aLine[nLen] = 0;
				memcpy(aTmpLine, aLine, nLen);
				UART_Puts("\r\n");
				cli_RunCmd(aLine);
				lb_NewEntry(aTmpLine);
				nLen = 0;
			}
			UART_Puts("\r\n$> ");
		}
		else if (0x7F == nCh) // backspace.
		{
			if (nLen > 0)
			{
				UART_Puts("\b \b");
				nLen--;
			}
		}
		else if (0x1B == nCh) // Escape sequence.
		{
			uint8 nCh2, nCh3;
			while (0 == UART_RxD((char*)&nCh2));
			if (0x5B == nCh2) // direction.
			{
				while (0 == UART_RxD((char*)&nCh3));
				if (0x41 == nCh3) // up.
				{
					nLen = lb_GetNextEntry(false, aLine);
					UART_Puts("\r                          \r->");
					if (nLen > 0) UART_Puts(aLine);
				}
				else if (0x42 == nCh3) // down.
				{
					nLen = lb_GetNextEntry(true, aLine);
					UART_Puts("\r                          \r+>");
					if (nLen > 0) UART_Puts(aLine);
				}
			}
		}
		else
		{
			CLI_Printf("~ %X\r\n", nCh);
		}
	}

	Sched_Wait(BIT(EVT_UART), 0);
}

/////////////////////
void CLI_Printf(char* szFmt, ...)
{
	char aBuf[64];
	va_list arg_ptr;
	va_start(arg_ptr, szFmt);
	vsprintf(aBuf, szFmt, arg_ptr);
	va_end(arg_ptr);
	cdc_puts(CDC_DBG, aBuf);
}

uint32 CLI_GetInt(char* szStr)
{
	uint32 nNum;
	char* pEnd;
	uint8 nLen = strlen(szStr);
	if ((szStr[0] == '0') && ((szStr[1] == 'b') || (szStr[1] == 'B'))) // Binary.
	{
		nNum = strtoul(szStr + 2, &pEnd, 2);
	}
	else
	{
		nNum = (uint32)strtoul(szStr, &pEnd, 0);
	}

	if ((pEnd - szStr) != nLen)
	{
		nNum = NOT_NUMBER;
	}
	return nNum;
}

void CLI_Register(char* szCmd, CmdHandler* pHandle)
{
	gaCmds[gnCmds].szCmd = szCmd;
	gaCmds[gnCmds].pHandle = pHandle;
	gnCmds++;
}

///////////////////////
void CLI_Init()
{
	UART_Init(UART_BPS);
	UART_SetCbf(cbf_RxUart, NULL);
	CLI_Register("help", cli_CmdHelp);
	CLI_Register("hist", cli_CmdHistory);
	Sched_Register(TID_CONSOLE, cli_Run);
}
