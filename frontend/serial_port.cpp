#include "serial_port.h"
#include <iostream>
#include <iomanip>

using namespace std;

serial_port::serial_port(io_addr base_addr, ConsoleLog& logg) : base_addr(base_addr), logg(logg),
THR(0),	RBR(0),	DLL(0),	IER(0),	DLH(0),	IIR(0),	FCR(0),	LCR(0),	MCR(0),	LSR(0),	MSR(0),	SR(0),
THR_addr(base_addr + 0x0), RBR_addr(base_addr + 0x0), DLL_addr(base_addr + 0x0), IER_addr(base_addr + 0x1),
DLH_addr(base_addr + 0x1), IIR_addr(base_addr + 0x2), FCR_addr(base_addr + 0x2), LCR_addr(base_addr + 0x3),
MCR_addr(base_addr + 0x4), LSR_addr(base_addr + 0x5), MSR_addr(base_addr + 0x6), SR_addr(base_addr + 0x7)
{

}

void serial_port::write_reg_byte(io_addr addr, uint8_t val) {
	//logg << "serial_port: ricevuto " << (char)val << "\n";

	if(addr==THR_addr) {
		logg << (char)val << std::flush;
		THR=addr;
	}
	else if(addr==DLL_addr) {
		DLL=addr;
	}
	else if(addr==IER_addr) {
		IER=addr;
	}
	else if(addr==DLH_addr) {
		DLH=addr;
	}
	else if(addr==FCR_addr) {
		FCR=addr;
	}
	else if(addr==LCR_addr) {
		LCR=addr;
	}
	else if(addr==MCR_addr) {
		MCR=addr;
	}
	else if(addr==SR_addr) {
		SR=addr;
	}

}

uint8_t serial_port::read_reg_byte(io_addr addr) {
	//logg << "serial_port: leggo " << std::hex << addr << endl;

	if(addr==RBR_addr) {
		return RBR;
	}
	else if(addr==DLL_addr) {
		return DLL;
	}
	else if(addr==IER_addr) {
		return IER;
	}
	else if(addr==DLH_addr) {
		return DLH;
	}
	else if(addr==IIR_addr) {
		return IIR;
	}
	else if(addr==LCR_addr) {
		return LCR;
	}
	else if(addr==MCR_addr) {
		return MCR;
	}
	else if(addr==LSR_addr) {
		// facciamo sembrare che la porta sia sempre disponibile a ricevere dati
		LSR |= 0x20;
		return LSR;
	}
	else if(addr==MSR_addr) {
		return MSR;
	}
	else if(addr==SR_addr) {
		return SR;
	}

	return 0;
}