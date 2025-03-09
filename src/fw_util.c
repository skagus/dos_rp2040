#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//#include "bsp/board.h"
#include "pico/stdlib.h"
//#include "tusb.h"
#include "uf2.h"

#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
//#include "lld_cdc.h"
#include "cli.h"
//------------- prototypes -------------//

#define BYTE_PER_PAGE		(256)
#define BYTE_PER_SECTOR		(4096)
#define PAGE_PER_SECTOR		(BYTE_PER_SECTOR / BYTE_PER_PATE)

#define FLASH_SIZE				(0x200000)  // 2MB.
#define BASE_FLASH_FW_ORG		(0x100000)  // 1MB.
#define BASE_FLASH_FW_COPY		(0x180000)  // 1.5MB.

bool check_all_ff(uint8_t* buffer, uint32_t bytes)
{
	uint32_t* buffer_4b = (uint32_t*)buffer;
	uint32_t size = bytes / sizeof(uint32_t);
	for(int i=0; i< size; i++)
	{
		if(0xFFFFFFFF != buffer_4b[i])
		{
			return false;
		}
	}
	return true;
}

void erase_flash(uint32_t offset, uint32_t size)
{
	uint32_t end_offset = offset + size;
	printf("Erase: %X ~ %X\n", offset, end_offset);
	if (end_offset >= offset + size)
	{

	}
//	busy_wait_ms (500);

	while(offset < end_offset)
	{
		//uint32_t interrupts = save_and_disable_interrupts();
		flash_range_erase(offset, BYTE_PER_SECTOR);
		//restore_interrupts(interrupts);

		offset += BYTE_PER_SECTOR;
	}
	printf("\nErase done\n");
}

void prog_flash(uint32_t offset, uint32_t size, uint8_t* data)
{
	uint32_t end_offset = offset + size;
	printf("PGM: %X ~ %X\n", offset, end_offset); 
	while(offset < end_offset)
	{
		//uint32_t interrupts = save_and_disable_interrupts();
		flash_range_program(offset, data, BYTE_PER_PAGE);
		//restore_interrupts(interrupts);

		offset += BYTE_PER_PAGE;
		data += BYTE_PER_PAGE;
	}
	printf("\nPGM Done\n");
}

void dump_flash(uint32_t offset, uint32_t size)
{
	const uint32_t read_size = BYTE_PER_PAGE;
	uint32_t data[read_size / 4];
	uint32_t end_offset = offset + size;

	printf("Flash Read: %X %X\n", XIP_BASE + offset, XIP_BASE + end_offset); 

	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();
		memcpy(data, (const uint8_t*)(XIP_BASE + offset), read_size);
		restore_interrupts(interrupts);

		for(int i=0; i< read_size / 4; i++)
		{
			if( i % 16 == 0)
			{
				printf("\n%8X :", offset + (i * 4));
			}
			printf("%08X ", data[i]);
		}
		offset += read_size;
	}
	printf("\nRead Done\n");
}


uint32_t handle_uf2_block(struct uf2_block* uf2)
{
	static uint32_t start_addr;
	printf("Rcv(%d/%d): %X, %d\n",
			uf2->block_no, uf2->num_blocks, uf2->target_addr, uf2->payload_size);

	if(0 == uf2->block_no)
	{
		uint32_t bytes = uf2->num_blocks * BYTE_PER_PAGE; 
		start_addr = uf2->target_addr;
		printf("erase 0x%X, 0x%X\n", BASE_FLASH_FW_COPY, bytes);
		erase_flash(BASE_FLASH_FW_COPY, bytes);
	}
	uint32_t offset = BASE_FLASH_FW_COPY + uf2->target_addr - start_addr;
	printf("program 0x%X, 0x%X\n", offset, uf2->payload_size);
	prog_flash(offset, uf2->payload_size, uf2->data);
	if(uf2->block_no == uf2->num_blocks - 1)
	{
		printf("UF2 Done\n");
	}
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+

int gb_RunUF2 = 1;

void cmd_UF2(uint8_t argc,char* argv[])
{
	gb_RunUF2 = (0 != gb_RunUF2) ? 0 : 1;
	printf("UF2 Run: %d\n", gb_RunUF2);
	return;
}

void cmd_DumpFlash(uint8_t argc,char* argv[])
{
	if(argc <= 2)
	{
		printf("read <address> <bytes>\n");
	}

	uint32_t base = CLI_GetInt(argv[1]);
	uint32_t size = CLI_GetInt(argv[2]);

	dump_flash(base, size);
}

void cmd_EraseFlash(uint8_t argc,char* argv[])
{
	if(argc <= 2)
	{
		printf("read <address> <bytes>\n");
	}

	uint32_t base = CLI_GetInt(argv[1]);
	uint32_t size = CLI_GetInt(argv[2]);

	erase_flash(base, size);
}

void cmd_Update(uint8_t argc, char* argv[])
{
	uint8_t buffer[BYTE_PER_PAGE];
	memcpy(buffer, (const uint8_t*)(XIP_BASE + BASE_FLASH_FW_COPY), BYTE_PER_PAGE);
	if(false == check_all_ff(buffer, BYTE_PER_PAGE))
	{
		printf("Start Update\n");
		uint32_t offset = 0;
		while(true)
		{
			memcpy(buffer, XIP_BASE + BASE_FLASH_FW_COPY + offset, BYTE_PER_PAGE);
			if(true == check_all_ff(buffer, BYTE_PER_PAGE))
			{
				printf("Migration done : 0x%X bytes\n", offset);
				uint32_t erase_off = 0;
				while(erase_off < offset)
				{
					erase_flash(BASE_FLASH_FW_COPY + erase_off, BYTE_PER_SECTOR);
					erase_off += BYTE_PER_SECTOR;
				}
				break;
			}
			if(0 == (offset % BYTE_PER_SECTOR))
			{
				erase_flash(BASE_FLASH_FW_ORG + offset, BYTE_PER_SECTOR);
			}
			prog_flash(BASE_FLASH_FW_ORG + offset, BYTE_PER_PAGE, buffer);
			offset += BYTE_PER_PAGE;
		}
	}
	else
	{
		printf("No FW available\n");
	}
}

#if 0
void FU_task()
{
	static int rcv_byte = 0;
	static uint8_t uf2_buf[512];
	static struct uf2_block* uf2 = (struct uf2_block*)uf2_buf;

	if ( tud_cdc_n_available(CDC_DATA) )
	{
		if(gb_RunUF2)
		{
			int avail = 512 - rcv_byte;
			uint32_t count = tud_cdc_n_read(CDC_DATA, uf2_buf + rcv_byte, avail);
			rcv_byte += count;

			if(rcv_byte >= 512)
			{
				handle_uf2_block(uf2);
				rcv_byte = 0;
			}
		}
		else
		{
			tud_cdc_n_read(CDC_DATA, uf2_buf, 512); // just rcv.
		}
	}
}
#endif

void FU_init()
{
	CLI_Register("uf2", cmd_UF2);
	CLI_Register("ota_update", cmd_Update);
	CLI_Register("flash_dump", cmd_DumpFlash);
	CLI_Register("flash_erase", cmd_EraseFlash);
}



