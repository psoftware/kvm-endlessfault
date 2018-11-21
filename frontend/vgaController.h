#ifndef VGACONTROLLER_H
#define VGACONTROLLER_H

#include "IODevice.h"
#include <stdint.h>
#include <pthread.h>
#include <iostream>
#include <math.h>
#include "../backend/ConsoleLog.h"

using namespace std;

// logger globale
extern ConsoleLog& logg;


class VGAController : public IODevice{

private:
	// === Registers ===
	uint8_t IND;	// it contains the selected register address
	uint8_t DAT;	// it contains the value to be read or written
	uint8_t CUR_HIGH; // it contains the value of the high-part of the cursor position
	uint8_t CUR_LOW; // it contains the value of the low-part of the cursor position

	// Registers Address 
	static const io_addr IND_addr = 0x03D4;
	static const io_addr DAT_addr = 0x03D5;
	static const uint8_t CUR_HIGH_ind = 0x0e;
	static const uint8_t CUR_LOW_ind = 0x0f;

	pthread_mutex_t mutex;

	uint16_t* _memoryStart;

public:

	VGAController();
	void write_reg_byte(io_addr addr, uint8_t val);
	uint8_t read_reg_byte(io_addr addr);
	uint16_t cursorPosition();

	void setVMem(uint16_t* mem);
	uint16_t* getVMem();

	// for network serialization
	bool field_serialize(netfields &nfields);
	bool field_deserialize(netfields &nfields);
};

#endif