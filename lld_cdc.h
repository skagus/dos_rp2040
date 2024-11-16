#pragma once

#include <stdint.h>

#define CDC_DBG		(0)
#define CDC_DATA	(1)

void cdc_puts(uint32_t itf, char* pBuf);
void cdc_printf(uint32_t itf,const char* fmt,...);
