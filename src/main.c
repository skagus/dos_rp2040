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

#include "lfs.h"
#include "dos.h"
#include "cli.h"
#include "fw_util.h"
//------------- prototypes -------------//


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+


static void cli_task(void)
{
	if (tud_cdc_n_available(CDC_DBG) )
	{
		char buf[64];
		uint32_t count = tud_cdc_n_read(CDC_DBG, buf, sizeof(buf));
		for(int index = 0; index < count; index++)
		{
			CLI_PushRcv(buf[index]);
		}
	}
}


/*------------- MAIN -------------*/
int main(void)
{
	board_init();
	tusb_init();
	CLI_Init();
	FU_init();
	DOS_Init();

	while (1)
	{
#if 0
		if(true == timer_fired )
		{ // reserve new alarm.
			printf("Alarm fired\n");
			timer_fired = false;
			add_alarm_in_ms(2000, alarm_callback, NULL, false);
		}
#endif
		//__wfi();  //__asm ("wfi");
		tud_task(); // tinyusb device task
		cli_task();
		FU_task();
	}

	return 0;
}

