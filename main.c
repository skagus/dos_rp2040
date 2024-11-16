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
#include "cli.h"

//------------- prototypes -------------//

#define BYTE_PER_PAGE		(256)
#define BYTE_PER_SECTOR		(4096)
#define PAGE_PER_SECTOR		(BYTE_PER_SECTOR / BYTE_PER_PATE)

void erase_flash(uint32_t offset, uint32_t end_offset)
{
	cdc_printf(CDC_DBG, "Erase: %X ~ %X\n", offset, end_offset); 
	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();

		flash_range_erase(offset, BYTE_PER_SECTOR);

		restore_interrupts(interrupts);
		offset += BYTE_PER_SECTOR;
	}
	cdc_printf(CDC_DBG, "Erase done\n");
}

void prog_flash(uint32_t offset, uint32_t end_offset, uint8_t* data)
{
	uint32_t data_tmp[BYTE_PER_PAGE / 4];
	if(NULL == data)
	{
		data = (uint8_t*)data_tmp;
		for(int i=0; i< BYTE_PER_PAGE/4; i++)
		{
			data[i] = offset + i;
		}
	}

	cdc_printf(CDC_DBG, "PGM: %X ~ %X\n", offset, end_offset); 
	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();

		flash_range_program(offset, data, BYTE_PER_PAGE);

		restore_interrupts(interrupts);
		offset += BYTE_PER_PAGE;
	}
	cdc_printf(CDC_DBG, "PGM Done\n");
}

void dump_flash(uint32_t offset, uint32_t end_offset)
{
	const uint32_t read_size = 256;
	uint32_t data[read_size / 4];

	cdc_printf(CDC_DBG, "RAM Read: %X %X\n", XIP_BASE + offset, XIP_BASE + end_offset); 

	while(offset < end_offset)
	{
		uint32_t interrupts = save_and_disable_interrupts();
		memcpy(data, (const uint8_t*)(XIP_BASE + offset), read_size);
		restore_interrupts(interrupts);

		for(int i=0; i< read_size / 4; i++)
		{
			if( i % 16 == 0)
			{
				cdc_printf(CDC_DBG, "\n%8X :", offset + (i * 4));
			}
			cdc_printf(CDC_DBG, "%08X ", data[i]);
		}
		offset += read_size;
	}
	cdc_printf(CDC_DBG, "Read Done\n");
}

#define BASE_FLASH_FW_COPY		(0x8000)
uint32_t handle_uf2_block(struct uf2_block* uf2)
{
	if(0 == uf2->block_no)
	{
		uint32_t bytes = uf2->num_blocks * BYTE_PER_PAGE; 
		erase_flash(BASE_FLASH_FW_COPY, bytes);	
	}
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static void cdc_task(void)
{
	static uint8_t buf_keep[512];
	static uint32_t n_valid = 0;
	static uint8_t cont = 1;

	uint8_t buf[128];
	
	if (tud_cdc_n_available(CDC_DBG) )
	{
		uint32_t count = tud_cdc_n_read(CDC_DBG, buf, sizeof(buf));
		if(buf[0] == 'C')
		{
			cont = 1;
			cdc_printf(CDC_DBG, "Cont: %d\n", cont);
		}
		else if (buf[0] == 'R')
		{
			dump_flash(0xB000, 0xD000);
		}
		else if (buf[0] == 'E')
		{
			erase_flash(0xC000, 0xD000);
		}
		else if (buf[0] == 'P')
		{
			prog_flash(0xC000, 0xD000, NULL);
		}
		else if (buf[0] == 'D')
		{
			cdc_printf(CDC_DBG, "Dump: %d\n", n_valid);
			for(int i=0; i< n_valid; i++)
			{
				while(tud_cdc_n_write_available(CDC_DBG) < 2)
				{
					tud_task();
				}
				tud_cdc_n_write_char(CDC_DBG, buf_keep[i]);
				tud_cdc_n_write_flush(CDC_DBG);
			}
			n_valid = 0;
		}
		else if(buf[0] == 'X')
		{
			cont = 0;
			cdc_printf(CDC_DBG, "Cont: %d\n", cont);
		}
	}

	if(cont == 1)  // RCV has a problem.
	{
		static int rcv_byte = 0;
		static uint8_t uf2_buf[512];
		static struct uf2_block* uf2 = (struct uf2_block*)uf2_buf;

		if ( tud_cdc_n_available(CDC_TEST) )
		{
#if 1
			int avail = 512 - rcv_byte;
			uint8_t* ptr = uf2_buf + rcv_byte;
			uint32_t count = tud_cdc_n_read(CDC_TEST, ptr, avail);
			rcv_byte += count;

			if(rcv_byte >= 512)
			{
				cdc_printf(CDC_DBG, "Rcv(%d/%d): %X, %d\n",
						uf2->block_no, uf2->num_blocks, uf2->target_addr, uf2->payload_size);
				handle_uf2_block(uf2);
				rcv_byte = 0;
			}
#else
			uint32_t count = tud_cdc_n_read(CDC_TEST, buf, sizeof(buf));
			buf[count] = 0;
			cdc_printf(CDC_DBG, "RCV: %d\n", count);

			for(int i=0; i< count; i++)
			{
				buf_keep[n_valid] = buf[i];
				n_valid++;
			}
#endif
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
	cdc_printf(0, "Repeat at %lld\n", time_us_64());
	return true;
}
#endif
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
			cdc_printf(0, "Alarm fired\n");
			timer_fired = false;
			add_alarm_in_ms(2000, alarm_callback, NULL, false);
		}
#endif
		tud_task(); // tinyusb device task
		cdc_task();
	}

	return 0;
}

