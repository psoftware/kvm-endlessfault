#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "IODevice.h"
#include <stdint.h>
#include <pthread.h>

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

	bool interrupt_raised;

	// mutex istanza (vale sia per frontend che backend)
	pthread_mutex_t mutex;

private:
	void process_cmd();

public:
	keyboard();

	void write_reg_byte(io_addr addr, uint8_t val);
	uint8_t read_reg_byte(io_addr addr);

	// serve al backend per pushare eventi di inserimento come handler
	void insert_keycode_event(uint8_t keycode);
};

#endif