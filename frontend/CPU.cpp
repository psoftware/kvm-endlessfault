#include "CPU.h"

using namespace std;

CPU::CPU(int vcpu_fd) : pending_stop(false) {
	this->vcpu_fd = vcpu_fd;
}

void CPU::save_registers() {
	if (ioctl(vcpu_fd, KVM_GET_REGS, &kregs) < 0) {
		logg << "CPU::save_registers KVM_GET_REGS error: " << strerror(errno) << endl;
		return;
	}

	if (ioctl(vcpu_fd, KVM_GET_SREGS, &ksregs) < 0) {
		logg << "CPU::save_registers KVM_GET_SREGS error: " << strerror(errno) << endl;
		return;
	}
}

bool CPU::field_serialize(netfields* &nfields) {
	nfields = new netfields(2);
	int f_id = 0;

	// === Registers ===
	nfields->set_field(f_id, (uint8_t*)(&kregs), sizeof(kregs)); f_id++;
	nfields->set_field(f_id, (uint8_t*)(&ksregs), sizeof(ksregs)); f_id++;

	return true;
}

bool CPU::field_deserialize(netfields &nfields) {
	// Check expected fields
	if(nfields.count != 2)
		return false;

	int f_id = 0;

	// === Registers ===
	nfields.get_field(f_id, (uint8_t*)(&kregs), sizeof(kregs)); f_id++;
	nfields.get_field(f_id, (uint8_t*)(&ksregs), sizeof(ksregs)); f_id++;

	return true;
}
