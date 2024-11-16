/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bsp/board.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "uf2.h"

#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "lld_cdc.h"

//------------- prototypes -------------//

#define PRINTF(fmt,...)		cdc_printf(CDC_DBG, fmt, ##__VA_ARGS__)

#define BYTE_PER_PAGE		(256)
#define BYTE_PER_SECTOR		(4096)
#define PAGE_PER_SECTOR		(BYTE_PER_SECTOR / BYTE_PER_PATE)

#define FLASH_SIZE				(0x200000)  // 2MB.
#define BASE_FLASH_FW_ORG		(0x100000)  // 1MB.
#define BASE_FLASH_FW_COPY		(0x180000)  // 1.5MB.

bool fw_update();


void erase_flash(uint32_t offset, uint32_t size)
{
	uint32_t end_offset = offset + size;
	PRINTF("Erase: %X ~ %X\n", offset, end_offset);
	if (end_offset >= offset + size)
	{

	}
//	busy_wait_ms (500);

	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();
		flash_range_erase(offset, BYTE_PER_SECTOR);
		restore_interrupts(interrupts);

		offset += BYTE_PER_SECTOR;
	}
	PRINTF("\nErase done\n");
}

void prog_flash(uint32_t offset, uint32_t size, uint8_t* data)
{
	uint32_t end_offset = offset + size;
	PRINTF("PGM: %X ~ %X\n", offset, end_offset); 
	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();
		flash_range_program(offset, data, BYTE_PER_PAGE);
		restore_interrupts(interrupts);

		offset += BYTE_PER_PAGE;
		data += BYTE_PER_PAGE;
	}
	PRINTF("\nPGM Done\n");
}

void dump_flash(uint32_t offset, uint32_t size)
{
	const uint32_t read_size = BYTE_PER_PAGE;
	uint32_t data[read_size / 4];
	uint32_t end_offset = offset + size;

	PRINTF("Flash Read: %X %X\n", XIP_BASE + offset, XIP_BASE + end_offset); 

	while(offset < end_offset)
	{
//		uint32_t interrupts = save_and_disable_interrupts();
		memcpy(data, (const uint8_t*)(XIP_BASE + offset), read_size);
//		restore_interrupts(interrupts);

		for(int i=0; i< read_size / 4; i++)
		{
			if( i % 16 == 0)
			{
				PRINTF("\n%8X :", offset + (i * 4));
			}
			PRINTF("%08X ", data[i]);
		}
		offset += read_size;
	}
	PRINTF("\nRead Done\n");
}


uint32_t handle_uf2_block(struct uf2_block* uf2)
{
	static uint32_t start_addr;
	PRINTF("Rcv(%d/%d): %X, %d\n",
			uf2->block_no, uf2->num_blocks, uf2->target_addr, uf2->payload_size);

	if(0 == uf2->block_no)
	{
		uint32_t bytes = uf2->num_blocks * BYTE_PER_PAGE; 
		start_addr = uf2->target_addr;
		PRINTF("erase 0x%X, 0x%X\n", BASE_FLASH_FW_COPY, bytes);
		erase_flash(BASE_FLASH_FW_COPY, bytes);
	}
	uint32_t offset = BASE_FLASH_FW_COPY + uf2->target_addr - start_addr;
	PRINTF("program 0x%X, 0x%X\n", offset, uf2->payload_size);
	prog_flash(offset, uf2->payload_size, uf2->data);
	if(uf2->block_no == uf2->num_blocks - 1)
	{
		PRINTF("UF2 Done\n");
	}
}

static void uf2_task(void)
{
	static int rcv_byte = 0;
	static uint8_t uf2_buf[512];
	static struct uf2_block* uf2 = (struct uf2_block*)uf2_buf;

	if ( tud_cdc_n_available(CDC_DATA) )
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
}
//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+

uint32_t parse_number(const char* str)
{
	if (str == NULL || *str == '\0')
	{
		return 0xFFFFFFFF; // 입력이 NULL이거나 빈 문자열인 경우 0 반환
	}
	// 16진수 (0x로 시작) 
	if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0)
	{
		return strtol(str, NULL, 16);
	}
	// 2진수 (0b로 시작)
	if (strncmp(str, "0b", 2) == 0 || strncmp(str, "0B", 2) == 0)
	{
		return strtol(str + 2, NULL, 2);
	}
	// 8진수 (0으로 시작)
	if (str[0] == '0' && strlen(str) > 1)
	{
		return strtol(str + 1, NULL, 8);
	}
	// 10진수 (그 외의 경우)
	return strtol(str, NULL, 10);
}

#define STR_CMP(name, input)	(0 == strncmp(name, input, strlen(name)))

int gb_RunUF2 = 1;

static void run_cmd(char* line)
{
	PRINTF("CMD: %s\n", line);

	char* tok = strtok(line, " ");
	PRINTF("TOK: %s\n", tok);
	do 
	{
		if (STR_CMP("uf2", tok))
		{
			gb_RunUF2 = (0 != gb_RunUF2) ? 0 : 1;
			PRINTF("UF2 Run: %d\n", gb_RunUF2);
			return;
		}

		if (STR_CMP("read", tok))
		{
			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t base = parse_number(tok);

			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t size = parse_number(tok);

			dump_flash(base, size);
			return;
		}

		if (STR_CMP("erase", tok))
		{
			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t base = parse_number(tok);

			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t size = parse_number(tok);

			erase_flash(base, size);
			return;
		}

		if (STR_CMP("prog", tok))
		{
			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t base = parse_number(tok);

			tok = strtok(NULL, " ");
			if(NULL == tok) break;
			uint32_t size = parse_number(tok);

			prog_flash(base, size, NULL);
			return;
		}
		if (STR_CMP("update", tok))
		{
			fw_update();
			return;
		}
	} while(0);

	PRINTF("Invalid command\n");
}

static void cdc_task(void)
{
	static uint32_t len = 0;
	static uint8_t line[128];

	if (tud_cdc_n_available(CDC_DBG) )
	{
		char buf[64];
		uint32_t count = tud_cdc_n_read(CDC_DBG, buf, sizeof(buf));
		
		strncpy(line + len, buf, count);
		len += count;
		buf[count] = '\0';
		PRINTF("%s", buf);

		char last = buf[count - 1];
		if(('\r' == last) || ('\n' == last))
		{
			len--;
			if(len > 0)
			{
				line[len] = 0; // remove last.
				run_cmd(line);
			}
			len = 0;
			PRINTF("$> ");
		}
	}
}

#if 0
/// \tag::timer_example[]
volatile bool timer_fired = false;

int64_t alarm_callback(alarm_id_t id, __unused void *user_data)
{
	timer_fired = true;
	// Can return a value here in us to fire in the future
	return 0;
}

bool repeating_timer_callback(__unused struct repeating_timer *t)
{
	PRINTF("Repeat at %lld\n", time_us_64());
	return true;
}
#endif
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


bool fw_update()
{
	uint8_t buffer[BYTE_PER_PAGE];
	memcpy(buffer, (const uint8_t*)(XIP_BASE + BASE_FLASH_FW_COPY), BYTE_PER_PAGE);
	if(false == check_all_ff(buffer, BYTE_PER_PAGE))
	{
		PRINTF("Start Update\n");
		uint32_t offset = 0;
		while(true)
		{
			memcpy(buffer, XIP_BASE + BASE_FLASH_FW_COPY + offset, BYTE_PER_PAGE);
			if(true == check_all_ff(buffer, BYTE_PER_PAGE))
			{
				PRINTF("Migration done : 0x%X bytes\n", offset);
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
		PRINTF("No FW available\n");
	}
	return true;
}

/*------------- MAIN -------------*/
int main(void)
{
	board_init();
	tusb_init();

	while (1)
	{
#if 0
		if(true == timer_fired )
		{ // reserve new alarm.
			PRINTF("Alarm fired\n");
			timer_fired = false;
			add_alarm_in_ms(2000, alarm_callback, NULL, false);
		}
#endif
//		__wfi();  //__asm ("wfi");
		tud_task(); // tinyusb device task
		cdc_task();
		if(1 == gb_RunUF2)
		{
			uf2_task();
		}
	}

	return 0;
}

