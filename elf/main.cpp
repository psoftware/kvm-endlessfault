#include "interp.h"
#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;
#define DIM 20000

char mem1[DIM];


int estrai_segmento(char *fname, void *dest)
{

	FILE* file;
	uint64_t entry_point;
	uint64_t last_address;

	if ( !(file = fopen(fname, "rb")) ) {
		perror(fname);
		exit(EXIT_FAILURE);
	}

	Eseguibile *exe = NULL;
	ListaInterpreti* interpreti = ListaInterpreti::instance();
	interpreti->rewind();
	while (interpreti->ancora()) {
		exe = interpreti->prossimo()->interpreta(file);
		if (exe) break;
	}
	if (!exe) {
		fprintf(stderr, "Formato del file '%s' non riconosciuto\n", fname);
		exit(EXIT_FAILURE);
	}

	entry_point = exe->entry_point();
	cout << "entry_point:" << std::hex << entry_point << endl;


	// dall'intestazione, calcoliamo l'inizio della tabella dei segmenti di programma
	last_address = 0;
	Segmento *s = NULL;
	while (s = exe->prossimo_segmento()) {
		uint64_t ind_virtuale = s->ind_virtuale();
		uint64_t dimensione = s->dimensione();
		uint64_t end_addr = ind_virtuale + dimensione;

		cout << "==> seg dim " << std::dec << dimensione << " addr " << std::hex <<
			ind_virtuale << "\n";

		/*if (end_addr > last_address)
			last_address = end_addr;

		ind_virtuale &= 0xfffffffffffff000;
		end_addr = (end_addr + 0x0000000000000fff) & 0xfffffffffffff000;*/
		cout << "byte copiati in m1 " << std::dec << s->copia_segmento(dest) << endl;

		for(int i=0; i<dimensione; i++){
			printf("%x",((char*)dest)[i]);
		}
		cout << endl;
	}
	fclose(file);
}

int main(int argc, char** argv)
{

	FILE* file;
	uint64_t entry_point;
	uint64_t last_address;
	char *fname;
	//
	if(argc < 2)
	{
		cout << "inserisci il filename." << endl;
		return -1;
	} else
		fname = argv[1];
	//
	estrai_segmento(fname,mem1);
}