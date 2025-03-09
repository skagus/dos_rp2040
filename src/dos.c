#include <stdint.h>
#include <stdio.h>

#include "hardware/flash.h"
#include "cli.h"
#include "dos.h"
#include "lfs.h"
#include "lfs_util.h"
#include "lld_cdc.h"

#define MAX_PATH_LEN		(260)
#define FS_LFS				(1)		// LittleFS or SPIFFS

extern struct lfs_config cfg;
extern lfs_t lfs;

// match your partition table.
#define FS_LABEL			"storage"

void _fs_format(uint8_t argc,char* argv[])
{
	printf("It takes a few seconds / minutes\n");
	int ret;
	ret = lfs_format(&lfs, &cfg);
	if(ret >= 0)
	{
		printf("Format done\n");
	}
	else
	{
		printf("Format failed\n");
	}
}

void _fs_mount(uint8_t argc,char* argv[])
{
	int ret = lfs_mount(&lfs, &cfg);
	if(ret >= 0)
	{
		printf("Mount done\n");
	}
	else
	{
		printf("Mount failed\n");
	}
}

#define FLASH_PAGE_SIZE		(256)
#define FLASH_SECTOR_SIZE	(4096)
extern uint32_t __flash_fs_start;
extern uint32_t __flash_fs_size;

#define FS_BASE		((uint32_t)(&__flash_fs_start) - XIP_BASE)
#define FS_SIZE		(uint32_t)(&__flash_fs_size)

void _fs_info(uint8_t argc,char* argv[])
{
#if 1
	printf("  XIP base: %X\n", XIP_BASE);
	printf("  FS base: %X\n", FS_BASE);
	printf("  FS size: %X\n", FS_SIZE);
	printf("  read_size: %d\n",cfg.read_size);
	printf("  prog_size: %d\n",cfg.prog_size);
	printf("  block_size: %d\n",cfg.block_size);
	printf("  block_count: %d\n",cfg.block_count);
	printf("  cache_size: %d\n",cfg.cache_size);
	printf("  lookahead_size: %d\n",cfg.lookahead_size);
	printf("  block_cycles: %d\n",cfg.block_cycles);
	printf("  read_buffer: %p\n",cfg.read_buffer);
	printf("  prog_buffer: %p\n",cfg.prog_buffer);
	printf("  lookahead_buffer: %p\n",cfg.lookahead_buffer);
	printf("  block_cycles: %d\n",cfg.block_cycles);
#else

	struct lfs_info info;
	int ret  = lfs_stat(&lfs, "/", &info);
	if(ret >= 0)
	{
		printf("Partition %s type: %X, size %d\n", 
					info.name, info.type, info.size);
	}
	else
	{
		printf("esp_spiffs_info error\n");
	}
#endif
}

#if 0
void _fs_write(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("Needs file name, ref 'copy con'\n");
		return;
	}

	char nRcv;
	// Use POSIX and C standard library functions to work with files.
	// First create a file.
	FILE* f = fopen(argv[1],"w");

	if(NULL != f)
	{
		while(1)
		{
			if(UART_RxByte(&nRcv, 0))
			{
				if(0x1A == nRcv) // Ctrl-Z
				{
					break;
				}
				printf("%c",nRcv);
				fputc(nRcv, f);
			}
		}

		fclose(f);
	}
	else
	{
		printf("Failed to open: %s\n",argv[1]);
	}
}

void _fs_list(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("needs directory name\n");
		return;
	}

	DIR* pDir = opendir(argv[1]);
	if(pDir != NULL)
	{
		char szPath[MAX_PATH_LEN];
		strcpy(szPath, argv[1]);
		strcat(szPath, "/");
		int nLenPath = strlen(szPath);
		struct dirent* pEntry;
		struct stat st;
		while((pEntry = readdir(pDir)) != NULL)
		{
			strcpy(szPath + nLenPath, pEntry->d_name);
			stat(szPath, &st);

			if(st.st_mode & S_IFDIR)
			{
				printf("%s (dir)\n", pEntry->d_name);
			}
			else 
			{
				snprintf(szPath,MAX_PATH_LEN,"%s/%s",argv[1],(char*)(pEntry->d_name));
				printf("%s (%ld bytes)\n", pEntry->d_name,st.st_size);
			}
		}
		closedir(pDir);
	}
	else
	{
		printf("directory open failed: %s\n", argv[1]);
	}
}

void _fs_delete(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("needs file name\n");
		return;
	}
	if(0 == unlink(argv[1]))
	{
		printf("delete %s done\n",argv[1]);
	}
	else
	{
		printf("delete %s failed\n",argv[1]);
	}
}

void _fs_rename(uint8_t argc,char* argv[])
{
	if(argc < 3)
	{
		printf("needs file name\n");
		return;
	}
	if(0 == rename(argv[1],argv[2]))
	{
		printf("rename %s to %s done\n",argv[1],argv[2]);
	}
	else
	{
		printf("rename %s to %s failed\n",argv[1],argv[2]);
	}
}

void _fs_mkdir(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("needs directory name\n");
		return;
	}
	if(0 == mkdir(argv[1],0))
	{
		printf("make directory %s done\n",argv[1]);
	}
	else
	{
		printf("make directory %s failed\n",argv[1]);
	}
}

void _fs_copy(uint8_t argc,char* argv[])
{
	if(argc < 3)
	{
		printf("needs source and destination file name\n");
		return;
	}

	FILE* pSrc = fopen(argv[1],"r");
	FILE* pDst = fopen(argv[2],"w");

	if(NULL != pSrc && NULL != pDst)
	{
		uint8_t aBuf[32];
		int nBytes;

		while(1)
		{
			nBytes = fread(aBuf,1,32,pSrc);
			if(nBytes <= 0)
			{
				break;
			}
			fwrite(aBuf,1,nBytes,pDst);
		}
		printf("Copy done: %s --> %s\n", argv[1], argv[2]);
	}
	else
	{
		printf("Error while open: %s or %s\n",argv[1],argv[2]);
	}
	if(NULL != pSrc) fclose(pSrc);
	if(NULL != pDst) fclose(pDst);
}

void _fs_read(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("needs file name\n");
		return;
	}

	FILE* pFile = fopen(argv[1],"r");

	if(NULL != pFile)
	{
		uint8_t aBuf[32];
		int nBytes;

		while(1)
		{
			nBytes = fread(aBuf,1,32,pFile);
			if(nBytes <= 0)
			{
				break;
			}
			for(uint8_t nIdx = 0; nIdx < nBytes; nIdx++)
			{
				printf("%c",aBuf[nIdx]);
			}
		}
		fclose(pFile);
	}
	else
	{
		printf("Error while open: %s\n",argv[1]);
	}
}


#define SIZE_DUMP_BUF		(32)
void _fs_dump(uint8_t argc,char* argv[])
{
	if(argc < 2)
	{
		printf("needs file name\n");
		return;
	}

	FILE* pFile = fopen(argv[1],"r");

	if(NULL != pFile)
	{
		uint8_t aBuf[SIZE_DUMP_BUF];
		int nBytes;
		uint32_t nCnt = 0;

		while(1)
		{
			nBytes = fread(aBuf,1,SIZE_DUMP_BUF,pFile);
			if(nBytes <= 0)
			{
				break;
			}
			printf(" %4lX : ",nCnt * SIZE_DUMP_BUF);
			for(uint8_t nIdx = 0; nIdx < nBytes; nIdx++)
			{
				printf("%02X ",aBuf[nIdx]);
			}
			nCnt++;
			printf("\n");
		}
		fclose(pFile);
	}
	else
	{
		printf("Error while open: %s\n",argv[1]);
	}
}
#endif

extern void my_lfs_init();

int DOS_Init()
{

	my_lfs_init();

	CLI_Register("info",_fs_info);
	CLI_Register("format",_fs_format);
	CLI_Register("mount",_fs_mount);
#if 0
	CLI_Register("cat",_fs_read);
	CLI_Register("fdump",_fs_dump);
	CLI_Register("fwrite",_fs_write);
	CLI_Register("ls",_fs_list);
	CLI_Register("rm",_fs_delete);
	CLI_Register("mv",_fs_rename);
	CLI_Register("mkdir",_fs_mkdir);
	CLI_Register("cp",_fs_copy);
#endif
	return 0;
}
