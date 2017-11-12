#include "IODevice.h"
#include <stdint.h>
#include <pthread.h>
#include <iostream>

using namespace std;

class VGAController : public IODevice{

private:
	// === Registri ===
	uint8_t IND;	// contiene l'indirizzo del registro selezionato
	uint8_t DAT;	// contiene il dato da leggere o scrivere nel registro selezionato
	uint8_t CUR_HIGH; // contiene il valore della parte alta della posizione del cursore
	uint8_t CUR_LOW; // contiene il valore della parte bassa della posizione del cursore

	// indirizzi registri
	static const io_addr IND_addr = 0x03D4;
	static const io_addr DAT_addr = 0x03D5;
	static const uint8_t CUR_HIGH_ind = 0x0e;
	static const uint8_t CUR_LOW_ind = 0x0f;

	pthread_mutex_t mutex;


public:

	VGAController();
	void write_reg_byte(io_addr addr, uint8_t val);

};