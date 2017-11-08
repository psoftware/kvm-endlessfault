#include "interp.h"
#include <iostream>
#include <cstdio>

using namespace std;

int main(int argc, char** argv)
{

	FILE* file;
	uint64_t entry_point;
	uint64_t last_address;
	//TabCache c;
	//entrata *e[5];
	char *fname;
	//
	if(argc < 2)
	{
		cout << "inserisci il filename." << endl;
		return -1;
	} else
		fname = argv[1];
	//

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


	// dall'intestazione, calcoliamo l'inizio della tabella dei segmenti di programma
	last_address = 0;
	Segmento *s = NULL;
	while (s = exe->prossimo_segmento()) {
		uint64_t ind_virtuale = s->ind_virtuale();
		uint64_t dimensione = s->dimensione();
		uint64_t end_addr = ind_virtuale + dimensione;

		cout << "==> seg dim " << std::hex << dimensione << " addr " <<
			ind_virtuale << "\n";

		if (end_addr > last_address)
			last_address = end_addr;

		ind_virtuale &= 0xfffffffffffff000;
		end_addr = (end_addr + 0x0000000000000fff) & 0xfffffffffffff000;
		/*for (; ind_virtuale < end_addr; ind_virtuale += sizeof(pagina))
		{
			log << "    addr " << std::hex << ind_virtuale << std::dec << "\n";
			block_t b;
			for (int l = 4; l > 1; l--) {
				int i = i_tabella(ind_virtuale, l);
				e[l] = &tab[l].e[i];
				log << "       T" << l << "[" << i << "] ->";
				if (e[l]->a.block == 0) {
					b = c.nuova(l - 1);
					log << " NEW";
					e[l]->a.block = b;
					e[l]->a.PWT   = 0;
					e[l]->a.PCD   = 0;
					e[l]->a.RW    = 1;
					e[l]->a.US    = liv;
					e[l]->a.P     = 0;
					c.scrivi(l);
				} else {
					c.leggi(l - 1, e[l]->a.block);
				}
				log << " T" << (l - 1) << " at " << e[l]->a.block << "\n";
			}

			int i = i_tabella(ind_virtuale, 1);
			e[1] = &tab[1].e[i];
			log << "       T1[" << i << "] ->";
			if (e[1]->a.block == 0) {
				if (! bm_alloc(&blocks, b) ) {
					fprintf(stderr, "%s: spazio insufficiente nello swap\n", fname);
					exit(EXIT_FAILURE);
				}
				e[1]->a.block = b;
				log << " NEW";
			} else {
				CHECKSW(leggi_blocco, e[1]->a.block, &pag);
			}
			if (s->pagina_di_zeri()) {
				e[1]->a.zeroed = 1;
				log << " zero";
			} else {
				s->copia_pagina(&pag);
				CHECKSW(scrivi_blocco, e[1]->a.block, &pag);
			}
			log << " page at " << e[1]->a.block << "\n";
			e[1]->a.PWT = 0;
			e[1]->a.PCD = 0;
			e[1]->a.RW |= s->scrivibile();
			e[1]->a.US |= liv;
			c.scrivi(1);
			s->prossima_pagina();
		}*/

	}
	fclose(file);
}