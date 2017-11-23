#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include "IODevice.h"
#include "../backend/ConsoleLog.h"
#include <stdint.h>

class serial_port {
private:
	// === Registri ===
	uint8_t THR;	// Transmitter Holding Buffer
	uint8_t RBR;	// Receiver Buffer
	uint8_t DLL;	// Divisor Latch Low Byte
	uint8_t IER;	// Interrupt Enable Register
	uint8_t DLH;	// Divisor Latch High Byte
	uint8_t IIR;	// Interrupt Identification Register
	uint8_t FCR;	// FIFO Control Register
	uint8_t LCR;	// Line Control Register
	uint8_t MCR;	// Modem Control Register
	uint8_t LSR;	// Line Status Register
	uint8_t MSR;	// Modem Status Register
	uint8_t SR;		// Scratch Register

	// indirizzi registri
	io_addr base_addr;

	// gli indirizzi li inizializziamo dal costruttore perchè sono un
	// offset di base_addr
	const uint16_t THR_addr;
	const uint16_t RBR_addr;
	const uint16_t DLL_addr;
	const uint16_t IER_addr;
	const uint16_t DLH_addr;
	const uint16_t IIR_addr;
	const uint16_t FCR_addr;
	const uint16_t LCR_addr;
	const uint16_t MCR_addr;
	const uint16_t LSR_addr;
	const uint16_t MSR_addr;
	const uint16_t SR_addr;

	// instanza di ConsoleLog per mostrare a video i risultati
	ConsoleLog& logg;

public:
	// costruttore
	serial_port(io_addr base_addr, ConsoleLog& logg);

	void write_reg_byte(io_addr addr, uint8_t val);
	uint8_t read_reg_byte(io_addr addr);
};

#endif