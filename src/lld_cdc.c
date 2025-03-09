#include <stdarg.h>
#include "tusb.h"
#include "lld_cdc.h"

#if 0
int _write(int file, char *ptr, int len)
{
	for(int i = 0; i < len; i++)
	{
		cdc_putchar(CDC_DBG, ptr[i]);
	}
	return len;
}


int printf(const char* fmt,...)
{
	char string[128];
	int len;

	va_list arg_ptr;
	va_start(arg_ptr,fmt);
	len = vsprintf(string,fmt,arg_ptr);
	va_end(arg_ptr);

	cdc_puts(CDC_DBG, string);
	return 0;
}

int putchar(int ch)
{
	cdc_putchar(CDC_DBG, (char)ch);
	return 0;
}
#endif

void cdc_printf(uint32_t itf, const char* fmt,...)
{
	char string[256];
	int len;

	va_list arg_ptr;
	va_start(arg_ptr,fmt);
	len = vsprintf(string,fmt,arg_ptr);
	va_end(arg_ptr);

	cdc_puts(itf, string);
}


void cdc_putchar(uint32_t itf, char ch)
{
	while(tud_cdc_n_write_available(itf) < 2)
	{
		tud_task();
	}
	tud_cdc_n_write_char(itf, ch);
	tud_cdc_n_write_flush(itf);
}

void cdc_puts(uint32_t itf, char* string)
{
	int len = strlen(string);

	for(uint32_t i = 0; i < len; i++)
	{
		while(tud_cdc_n_write_available(itf) < 2)
		{
			tud_task();
		}
		tud_cdc_n_write_char(itf,string[i]);
	}
	tud_cdc_n_write_flush(itf);
}
