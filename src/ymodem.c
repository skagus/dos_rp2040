#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"

#include "lfs.h"
#include "crc16.h"
#include "cli.h"
#include "ymodem.h"


#define MODEM_SEQ_SIZE		2
#define DATA_SIZE_ORG		128
#define DATA_SIZE_EXT		1024
#define MODEM_CRC_SIZE		2
#define TIMEOUT				3000

#define LOG_NAME		"/ym.log"

extern lfs_t lfs;
uint8_t aBuf[MODEM_SEQ_SIZE + DATA_SIZE_EXT + MODEM_CRC_SIZE + 10];

void tx_byte(char nCh)
{
	//putchar(nCh);
	stdio_putchar_raw(nCh);
}

/* return  true if a char received*/
bool rx_byte(char* pCh, uint32_t nToMsec)
{
	int data = getchar_timeout_us(nToMsec * 1000);
	if(data < 0) return false;

	*pCh = (char)data;
	return true;
}

void tx_packet(uint8_t* aBuf, uint32_t nSize)
{
	for(int i=0; i< nSize; i++)
	{
		tx_byte(aBuf[i]);
	}
	stdio_flush();
}

bool rx_packet(uint8_t* aBuf, uint32_t nSize)
{
	uint8_t nCh;
	uint32_t nIdx = 0;
	while(nIdx < nSize)
	{
		if(rx_byte((char*)&nCh, 2 * TIMEOUT) == 0)
		{
			log_printf("%d: TO %d/%d\n", __LINE__, nIdx, nSize);
			for(uint32_t i = 0; i < nIdx; i++)
			{
				log_printf("%2X ", aBuf[i]);
			}
			log_printf("\n");
			return false;
		}
		aBuf[nIdx] = nCh;
		nIdx++;
	}
	return true;
}

void log_init(char* szpath)
{
//	lfs_file_open(&lfs, &log_file, szpath, LFS_O_CREAT | LFS_O_WRONLY);
}

char g_log_buf[1024];
int ptr = 0;
void log_printf(const char* pFmt, ...)
{
#if 1
	va_list args;
	va_start(args, pFmt);
	int len = vsprintf(g_log_buf + ptr, pFmt, args);
	ptr += len;	
	va_end(args);
#else
	lfs_file_t log_file;
	char buffer[256];
	int len;
	lfs_file_open(&lfs, &log_file, LOG_NAME, LFS_O_WRONLY | LFS_O_APPEND);

	va_list args;
	va_start(args, pFmt);
	len = vsprintf(buffer, pFmt, args);
	lfs_file_write(&lfs, &log_file, buffer, len);
	va_end(args);

	lfs_file_close(&lfs, &log_file);
#endif
}

void log_close()
{
//	fclose(gf_log);
}


bool expactRcvByte(uint8_t nExp)
{
	uint8_t nCh;
	if(false == rx_byte((char*)&nCh, TIMEOUT))
	{
		log_printf("%d: TO exp:%X\n", __LINE__, nExp);
		return false;
	}
	log_printf("%d: Exp %X, Rcv: %X\n", __LINE__, nExp, nCh);
	if(nCh != nExp)
	{
		return false;
	}

	return true;
}


bool _ParityCheck(bool bCRC, const uint8_t* aBuf, int nDataSize)
{
	if(bCRC)
	{
		uint16_t crc = CRC16_ccitt(aBuf, nDataSize);
		uint16_t tcrc = (aBuf[nDataSize] << 8) + aBuf[nDataSize + 1];
		if(crc == tcrc) return true;
	}
	else
	{
		uint8_t nChkSum = 0;
		for(int i = 0; i < nDataSize; ++i)
		{
			nChkSum += aBuf[i];
		}
		if(nChkSum == aBuf[nDataSize]) return true;
	}

	return false;
}

int err_code;
#define ERROR(x)		{err_code=x;goto ERROR_RET;}

void YM_Rcv(char* szTopDir)
{
	uint8_t* pData = &aBuf[MODEM_SEQ_SIZE];
	char aFullPath[256];
	uint8_t nCh;
	uint8_t nSeq = 0;
	uint32_t nDataLen;
	uint32_t nWritten = 0;

	lfs_file_t file;

	uint32_t nDirLen = strlen(szTopDir);
	strcpy(aFullPath, szTopDir);
	if(aFullPath[nDirLen - 1] != '/')
	{
		aFullPath[nDirLen++] = '/';
		aFullPath[nDirLen] = 0;
	}
	printf("Start RX to %s\n", aFullPath);

	log_printf("%3d, Start!!!!\n", __LINE__);

	// Wait Start.
	while(true)
	{
		tx_byte(MODEM_C);
		if(rx_byte((char*)&nCh, TIMEOUT))
		{
			if(nCh == MODEM_SOH) nDataLen = DATA_SIZE_ORG;
			else if(nCh == MODEM_STX) nDataLen = DATA_SIZE_EXT;
			else ERROR(__LINE__);
			break;
		}
	}

	// log_printf("%3d, LEN: %d\n", __LINE__, nDataLen);
	// Receive Header.
	if(rx_packet(aBuf, MODEM_SEQ_SIZE + nDataLen + MODEM_CRC_SIZE) == false) ERROR(__LINE__);

	if(aBuf[0] != nSeq) ERROR(__LINE__);
	if(aBuf[1] != (0xFF - nSeq)) ERROR(__LINE__);
	if(!_ParityCheck(true, pData, nDataLen)) ERROR(__LINE__);
	// do something with file name.
	strcpy(aFullPath + nDirLen, (char*)pData);
	int fileSize = atoi((char*)pData + strlen((char*)pData) + 1);
	int nRestSize = fileSize;
	log_printf("%3d: file name: %s, size: %s, %d\n",
		__LINE__, pData, pData + strlen((char*)pData) + 1, fileSize);

	if(nRestSize <= 0) nRestSize = INT32_MAX;

	if(lfs_file_open(&lfs, &file, aFullPath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND) < 0)
	{
		ERROR(__LINE__);
	}
	////////
	tx_byte(MODEM_ACK);
	tx_byte(MODEM_C);
	nSeq++;

	while(true)
	{
		if(rx_byte((char*)&nCh, TIMEOUT) == 0) ERROR(__LINE__);

		if(nCh == MODEM_EOT)
		{
			lfs_file_close(&lfs, &file);
			log_printf("FClose %d\n", nSeq);
			break;
		}
		else if(nCh == MODEM_SOH) nDataLen = DATA_SIZE_ORG;
		else if(nCh == MODEM_STX) nDataLen = DATA_SIZE_EXT;
		else ERROR(__LINE__); // if(nCh == MODEM_CAN) goto ERROR;

		if(!rx_packet(aBuf, MODEM_SEQ_SIZE + nDataLen + MODEM_CRC_SIZE)) ERROR(__LINE__);
		if(aBuf[0] != nSeq) ERROR(__LINE__);
		if(aBuf[1] != (0xFF - nSeq)) ERROR(__LINE__);
		if(!_ParityCheck(true, pData, nDataLen)) ERROR(__LINE__);
		// do something with data.
		if(nRestSize < nDataLen)
		{
			nDataLen = nRestSize;
		}
#if 0
		#define MAX_WRITE_SIZE	(128)
		int nRest = nDataLen;
		uint8_t* pBuf = pData;
		while(nRest > 0)
		{
			int thisLen = nRest > MAX_WRITE_SIZE ? MAX_WRITE_SIZE : nRest;
			int len = lfs_file_write(&lfs, &file, pBuf, thisLen);
			log_printf("W %d=%d\n", thisLen, len);
			pBuf += thisLen;
			nRest -= thisLen;
		}
#else		
		int len = lfs_file_write(&lfs, &file, pData, nDataLen);
		log_printf("Write %d:%d\n", nSeq, len);
#endif

		nWritten += nDataLen;
		///
		tx_byte(MODEM_ACK);
		log_printf("%3d SEQ: %d\n", __LINE__, nSeq);
		nSeq++;
		nRestSize -= nDataLen;
	}

	log_printf("%3d\n", __LINE__);

	/// End Sequence.
	tx_byte(MODEM_NACK);
	if(!expactRcvByte(MODEM_EOT)) ERROR(__LINE__);
	tx_byte(MODEM_ACK);

	log_printf("%3d\n", __LINE__);

	// Receive Last Packet.
	tx_byte(MODEM_C);
	if(!expactRcvByte(MODEM_SOH)) ERROR(__LINE__);
	log_printf("%3d\n", __LINE__);
	if(!rx_packet(aBuf, MODEM_SEQ_SIZE + DATA_SIZE_ORG + MODEM_CRC_SIZE)) ERROR(__LINE__);
	if(aBuf[0] != 0x00) ERROR(__LINE__);
	if(aBuf[1] != 0xFF) ERROR(__LINE__);
	tx_byte(MODEM_ACK);

	log_printf("Success: %s (%d, %d)\n", aFullPath, fileSize, nWritten);
	printf("Success: %s (%d, %d)\n", aFullPath, fileSize, nWritten);
	return;
	
ERROR_RET:
	lfs_file_close(&lfs, &file);
	tx_byte(MODEM_CAN);
	tx_byte(MODEM_CAN);
	tx_byte(MODEM_CAN);
	log_printf("RET Error!!!: %d\n", err_code);
	printf("\n\nError: %d\n", err_code);
}


void _ParityGen(bool bCRC, uint8_t* aBuf, int nDataSize)
{
	if(bCRC)
	{
		uint16_t crc = CRC16_ccitt(aBuf, nDataSize);
		aBuf[nDataSize] = crc >> 8;
		aBuf[nDataSize + 1] = crc & 0xFF;
	}
	else
	{
		uint8_t nChkSum = 0;
		for(int i = 0; i < nDataSize; ++i)
		{
			nChkSum += aBuf[i];
		}
		aBuf[nDataSize] = nChkSum;
	}
}

int _getFileSize(char* szPath)
{
	struct lfs_info info;
	if(lfs_stat(&lfs, szPath, &info) < 0) return -1;
	return info.size;
}
	
void YM_Snd(char* szFullPath)
{
	uint8_t* pData = &aBuf[MODEM_SEQ_SIZE];
	uint8_t nSeq = 0;
	uint32_t nSpaceLen = DATA_SIZE_ORG;
	uint32_t nDataLen;
	lfs_file_t file;
	int nFileSize = _getFileSize(szFullPath);
	int nRestSize = nFileSize;

	if(lfs_file_open(&lfs, &file, szFullPath, LFS_O_RDONLY) < 0) goto ERROR;

	char* szFileName = strrchr(szFullPath, '/');
	if(szFileName == NULL) szFileName = szFullPath;
	else szFileName++;

	printf("Start TX: %s (%d)\n", szFullPath, nFileSize);

	// Wait Start.
	int nRestTry = 20;
	while(nRestTry-- > 0)
	{
		if(expactRcvByte(MODEM_C)) break;
	}
	if(nRestTry <= 0) goto ERROR;

	while(true) // Remove dummy.
	{
		if(rx_byte((char*)&nSeq, 100))
		{
			log_printf("Dummy: %X\n", nSeq);
		}
		else
		{
			break;
		}
	}

	// Prepare Header.
	aBuf[0] = nSeq;
	aBuf[1] = 0xFF - nSeq;
	nDataLen = strlen(szFileName);
	memcpy(pData, szFileName, nDataLen);
	sprintf((char*)pData + nDataLen + 1, "%d", nRestSize);
//	memset(pData + nDataLen, 0, nSpaceLen - nDataLen);
	_ParityGen(true, pData, nSpaceLen);
	tx_byte(MODEM_SOH);
	tx_packet(aBuf, MODEM_SEQ_SIZE + nSpaceLen + MODEM_CRC_SIZE);
	if(!expactRcvByte(MODEM_ACK)) goto ERROR;
	if(!expactRcvByte(MODEM_C)) goto ERROR;

	log_printf("Head done Name: %s, Size: %d\n", szFileName, nRestSize);
	nSeq++;
	while(true)
	{
		nSpaceLen = DATA_SIZE_EXT;
		aBuf[0] = nSeq;
		aBuf[1] = 0xFF - nSeq;
		nDataLen = lfs_file_read(&lfs, &file, pData, nSpaceLen);
		nRestSize -= nDataLen;
		if(nDataLen <= 0)
		{
			lfs_file_close(&lfs, &file);
			break;
		}
#if 0
		if(nDataLen < DATA_SIZE_ORG)
		{
			nSpaceLen = DATA_SIZE_ORG;
		}
#endif
//		memset(pData + nDataLen, 0, nSpaceLen - nDataLen);
		_ParityGen(true, pData, nSpaceLen);
		tx_byte(MODEM_STX);
		tx_packet(aBuf, MODEM_SEQ_SIZE + nSpaceLen + MODEM_CRC_SIZE);
		if(!expactRcvByte(MODEM_ACK)) goto ERROR;
		log_printf("SEQ %d done\n", nSeq);

		nSeq++;
	}

	// End Sequence.
	tx_byte(MODEM_EOT);
	if(!expactRcvByte(MODEM_NACK)) goto ERROR;
	tx_byte(MODEM_EOT);
	if(!expactRcvByte(MODEM_ACK)) goto ERROR;
	if(!expactRcvByte(MODEM_C)) goto ERROR;

	log_printf("End SEQ\n");

	// Send Last Packet.
	aBuf[0] = 0x00;
	aBuf[1] = 0xFF;
	tx_byte(MODEM_SOH);
	memset(pData, 0, DATA_SIZE_ORG);
	_ParityGen(true, pData, DATA_SIZE_ORG);
	tx_packet(aBuf, MODEM_SEQ_SIZE + DATA_SIZE_ORG + MODEM_CRC_SIZE);
	if(!expactRcvByte(MODEM_ACK)) goto ERROR;

	printf("\nSuccess: %s, (%d)\n", szFullPath, nFileSize);
	log_printf("\nSuccess: %s, (%d)\n", szFullPath, nFileSize);
	return;

ERROR:
//	lfs_file_close(&lfs, &file);

	tx_byte(MODEM_CAN);
	tx_byte(MODEM_CAN);
	tx_byte(MODEM_CAN);
	log_printf("%3d: Error!!!\n", __LINE__);
	
	while(rx_byte((char*)&nSeq, 100))
	{
		log_printf("E Dummy: %X\n", nSeq);
	}
	printf("\n\nError\n");
}

void cmd_RcvY(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("Needs top directory\n");
		return;
	}
	YM_Rcv(argv[1]);
}

void cmd_SndY(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("Needs file name with path\n");
		return;
	}
	YM_Snd(argv[1]);
}

void cmd_Test(uint8_t argc,char* argv[])
{
#if 1
	puts(g_log_buf);
	ptr = 0;
#else
	char nRcv;
	if(rx_byte(&nRcv, 2000))
	{
		printf("Rcv: %c\n", nRcv);
	}
	else
	{
		printf("Timeout\n");
	}
#endif
}

void YM_Init()
{
	CLI_Register("ymt", cmd_Test);
	CLI_Register("ry", cmd_RcvY);
	CLI_Register("sy", cmd_SndY);
}

