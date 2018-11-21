#ifndef IODEVICE_H
#define IODEVICE_H

#include <stdint.h>

#include "../netlib/FieldSerializable.h"

typedef uint16_t io_addr;

class IODevice : public FieldSerializable {
	virtual void write_reg_byte(io_addr addr, uint8_t val);
	virtual void write_reg_word(io_addr addr, uint16_t val);
	virtual void write_reg_long(io_addr addr, uint32_t val);
	virtual uint8_t read_reg_byte(io_addr addr);
	virtual uint16_t read_reg_word(io_addr addr);
	virtual uint32_t read_reg_long(io_addr addr);
};

#endif