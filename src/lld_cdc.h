#pragma once

#include <stdint.h>

#define CDC_DBG		(0)
#define CDC_DATA	(1)

void cdc_putchar(uint32_t itf, char ch);
void cdc_puts(uint32_t itf, char* pBuf);
void cdc_printf(uint32_t itf,const char* fmt,...);

#define printf(...) cdc_printf(CDC_DBG, __VA_ARGS__)
#define putchar(...) cdc_putchar(CDC_DBG, __VA_ARGS__)
#define puts(...) cdc_puts(CDC_DBG, __VA_ARGS__)
