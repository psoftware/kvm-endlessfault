#include "interp.h"
#include <iostream>
#include <cstdio>
#include <cstring>

using namespace std;

uint32_t estrai_segmento(char *fname, void *dest, uint64_t dest_size)
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

		// ==== DA VALUTARE SE VA BENE
		if(end_addr > dest_size)
		{
			cout << "il segmento è troppo grande per essere caricato o sfora i limiti della memoria fisica" << endl;
			continue;
		}
		// ====

		cout << "byte copiati in m1 " << std::dec << s->copia_segmento((uint8_t*)dest + ind_virtuale) << endl;

		for(int i=ind_virtuale; i<ind_virtuale+dimensione; i++){
			printf("%x",((unsigned char*)dest)[i]);
		}
		cout << endl;
	}
	fclose(file);

	return entry_point;
}
