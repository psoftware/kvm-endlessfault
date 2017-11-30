#ifndef LIB_H_
#define LIB_H_

typedef unsigned short ioaddr;

void shutdown();
void exit(unsigned int result);
void inputb(ioaddr reg, unsigned char *a);
void outputb(unsigned char a, ioaddr reg);

#endif