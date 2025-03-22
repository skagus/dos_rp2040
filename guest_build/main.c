//#include <string.h>
#include "rom_sym.h"
int gnBSS;
int gnData = 0xFA;

int main(int argc, char* argv[])
{
	gnBSS += gnData;

	puts("In Guest Done\n");
	printf("argc:%d\n", argc);
	for(int i=0; i< argc; i++)
	{
		printf("argv[%d]:%s\n", i, argv[i]);
	}
	return (gnBSS);
}
