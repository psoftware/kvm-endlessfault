#include "lib.h"

int main()
{
	unsigned int a=2, b=10;
	unsigned int c = a+b;
	unsigned d = 100*1024*1024; // 100MiB
	char *ptr = (char*)d;
	char k = *ptr;	


	exit(c);

	// non ci arriviamo mai
	return c;
}
