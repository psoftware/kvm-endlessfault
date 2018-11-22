#include "CPU.h"

using namespace std;

CPU::CPU(int vcpu_fd) : pending_stop(false) {
	this->vcpu_fd = vcpu_fd;

	/*memset(&kregs, 0, sizeof(kregs));
	memset(&ksregs, 0, sizeof(ksregs));
	memset(&kmsrs, 0, sizeof(kmsrs));
	memset(&kcpuevents, 0, sizeof(kcpuevents));*/

	save_registers();
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

	/*if (ioctl(vcpu_fd, KVM_GET_MSRS, &kmsrs) < 0) {
		logg << "CPU::save_registers KVM_GET_MSRS error: " << strerror(errno) << endl;
		return;
	}*/

	if (ioctl(vcpu_fd, KVM_GET_VCPU_EVENTS, &kcpuevents) < 0) {
		logg << "CPU::save_registers KVM_GET_VCPU_EVENTS error: " << strerror(errno) << endl;
		return;
	}
}

void CPU::load_registers() {
	if (ioctl(vcpu_fd, KVM_SET_REGS, &kregs) < 0) {
		logg << "CPU::load_registers KVM_SET_REGS error: " << strerror(errno) << endl;
		return;
	}

	if (ioctl(vcpu_fd, KVM_SET_SREGS, &ksregs) < 0) {
		logg << "CPU::load_registers KVM_SET_SREGS error: " << strerror(errno) << endl;
		return;
	}

	if (ioctl(vcpu_fd, KVM_SET_MSRS, &kmsrs) < 0) {
		logg << "CPU::load_registers KVM_SET_MSRS error: " << strerror(errno) << endl;
		return;
	}

	if (ioctl(vcpu_fd, KVM_SET_VCPU_EVENTS, &kcpuevents) < 0) {
		logg << "CPU::load_registers KVM_GET_VCPU_EVENTS error: " << strerror(errno) << endl;
		return;
	}
}

bool CPU::field_serialize(netfields* &nfields) {
	nfields = new netfields(3);
	int f_id = 0;

	// === Registers ===
	nfields->set_field(f_id, (uint8_t*)(&kregs), sizeof(kregs)); f_id++;
	nfields->set_field(f_id, (uint8_t*)(&ksregs), sizeof(ksregs)); f_id++;
	//nfields->set_field(f_id, (uint8_t*)(&kmsrs), sizeof(kmsrs)); f_id++;
	nfields->set_field(f_id, (uint8_t*)(&kcpuevents), sizeof(kcpuevents)); f_id++;

	/*logg << "\tRIP: " << (void *)kregs.rip << endl;
	logg << "\tRSP: " << (void *)kregs.rsp << endl;

	logg << "\tRAX: " << (void *)kregs.rax << endl;
	logg << "\tRBX: " << (void *)kregs.rbx << endl;
	logg << "\tRCX: " << (void *)kregs.rcx << endl;
	logg << "\tRDX: " << (void *)kregs.rdx << endl;

	logg << "\tCR4: " << (void *)ksregs.cr4 << endl;
	logg << "\tCR3: " << (void *)ksregs.cr3 << endl;
	logg << "\tCR2: " << (void *)ksregs.cr2 << endl;
	logg << "\tCR0: " << (void *)ksregs.cr0 << endl;
	logg << "\tEFER: " << (void *)ksregs.efer << endl;

	logg << "\tIDT base: " << (void *)ksregs.idt.base << endl;
	logg << "\tIDT limit: " << (unsigned int)ksregs.idt.limit << endl;
	logg << "\tGDT base: " << (void *)ksregs.gdt.base << endl;
	logg << "\tGDT limit: " << (unsigned int)ksregs.gdt.limit << endl;*/

	return true;
}

bool CPU::field_deserialize(netfields &nfields) {
	// Check expected fields
	if(nfields.count != 3)
		return false;

	int f_id = 0;

	// === Registers ===
	if(!nfields.get_field(f_id, (uint8_t*)(&kregs), sizeof(kregs)))
		return false; f_id++;
	if(!nfields.get_field(f_id, (uint8_t*)(&ksregs), sizeof(ksregs)))
		return false; f_id++;
	/*if(!nfields.get_field(f_id, (uint8_t*)(&kmsrs), sizeof(kmsrs)))
		return false; f_id++;*/
	if(!nfields.get_field(f_id, (uint8_t*)(&kcpuevents), sizeof(kcpuevents)))
		return false; f_id++;

	/*logg << "\tRIP: " << (void *)kregs.rip << endl;
	logg << "\tRSP: " << (void *)kregs.rsp << endl;

	logg << "\tRAX: " << (void *)kregs.rax << endl;
	logg << "\tRBX: " << (void *)kregs.rbx << endl;
	logg << "\tRCX: " << (void *)kregs.rcx << endl;
	logg << "\tRDX: " << (void *)kregs.rdx << endl;

	logg << "\tCR4: " << (void *)ksregs.cr4 << endl;
	logg << "\tCR3: " << (void *)ksregs.cr3 << endl;
	logg << "\tCR2: " << (void *)ksregs.cr2 << endl;
	logg << "\tCR0: " << (void *)ksregs.cr0 << endl;
	logg << "\tEFER: " << (void *)ksregs.efer << endl;

	logg << "\tIDT base: " << (void *)ksregs.idt.base << endl;
	logg << "\tIDT limit: " << (unsigned int)ksregs.idt.limit << endl;
	logg << "\tGDT base: " << (void *)ksregs.gdt.base << endl;
	logg << "\tGDT limit: " << (unsigned int)ksregs.gdt.limit << endl;*/

	return true;
}
