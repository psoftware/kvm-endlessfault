#include <iostream>
using namespace std;

extern unsigned long estrai_segmento(char *fname, void *dest, unsigned long dest_size);
#define DIM 20000

unsigned char mem1[DIM];

int main(int argc, char** argv)
{
	char *fname;
	if(argc < 2)
	{
		cout << "inserisci il filename." << endl;
		return -1;
	} else
		fname = argv[1];
	estrai_segmento(fname,mem1,DIM);

	return 0;
}