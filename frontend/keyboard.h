#ifndef KEYBOARD_H
#define KEYBOARD_H

#define INTERNAL_BUFFER_SIZE 16

#include "IODevice.h"
#include <stdint.h>
#include <pthread.h>
#include "../backend/ConsoleLog.h"

// logger globale
extern ConsoleLog& logg;

class keyboard : public IODevice {
private:
	// === Registri ===
	uint8_t RBR;	// buffer di ingresso
	uint8_t TBR;	// buffer di uscita
	uint8_t STR;	// registro di stato
	uint8_t CMR;	// registro di comando

	// indirizzi registri
	static const io_addr RBR_addr = 0x0060;
	static const io_addr TBR_addr = 0x0060;
	static const io_addr STR_addr = 0x0064;
	static const io_addr CMR_addr = 0x0064;

	// flag per registro STR
	static const uint8_t FI_MASK = 1u;
	static const uint8_t FO_MASK = 1u << 1;

	// === Stato interno ===
	bool enabled;
	bool interrupt_enabled;

	// buffer interno (l'hardware dell'Intel 8082 lo prevede)
	uint8_t internal_buffer[INTERNAL_BUFFER_SIZE];
	short buffer_head_pointer;
	short buffer_tail_pointer;
	short buffer_element_count;

	bool interrupt_raised;

	// mutex istanza (vale sia per frontend che backend)
	pthread_mutex_t mutex;

private:
	void process_cmd();

	// questo metodo deve essere chiamato prima di una lettura su RBR
	// per aggiornare il contenuto del registro RBR e STR
	void next_RBR();

	// questo metodo è chiamato per aggiornare FI in base allo stato
	// del buffer (vuoto/non vuoto)
	void update_FI();

public:
	keyboard();

	void write_reg_byte(io_addr addr, uint8_t val);
	uint8_t read_reg_byte(io_addr addr);

	// serve al backend per pushare eventi di inserimento come handler
	void insert_keycode_event(uint8_t keycode);

	// for network serialization
	bool field_serialize(netfields* &nfields);
	bool field_deserialize(netfields &nfields);
};

#endif