#ifndef CPU_H_
#define CPU_H_

#include <linux/kvm.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <iostream>

#include "../netlib/commlib.h"
#include "../netlib/FieldSerializable.h"
#include "../backend/ConsoleLog.h"

// global logger
extern ConsoleLog& logg;

class CPU : public FieldSerializable {
private:
	int vcpu_fd;
	kvm_regs kregs;
	kvm_sregs ksregs;

public:
	// vcpu can be interrupted while KVM_EXIT, this is to avoid to re-enter KVM_RUN
	bool pending_stop;

	CPU(int vcpu_fd);

	void save_registers();
	bool field_serialize(netfields* &nfields);
	bool field_deserialize(netfields &nfields);
};

#endif