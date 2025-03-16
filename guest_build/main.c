#include <string.h>

int gnBSS;
int gnData = 0xFA;

char gaRetBuffer[128];

int main(int argc, char* argv[])
{
	gnBSS = gnData - 10;
	char* buf = gaRetBuffer;
	buf += sprintf(buf, "argc:%d\n", argc);
	for(int i=0; i< argc; i++)
	{
		buf += sprintf(buf, "argv[%d]:%s\n", i, argv[i]);
	}
	return (int)gaRetBuffer;
}


