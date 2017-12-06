#ifndef BOOT_H
#define BOOT_H

#include <linux/kvm.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

class Bootloader {
private:
	int vcpu_fd_;
	unsigned char *guest_mem_;
	uint32_t guest_mem_size_;
	uint64_t entry_point_;
	// Used just in protected mode. In long mode extern bootloader sets its own stack
	uint64_t start_stack_;
	void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg);
	void setup_protected_mode(struct kvm_sregs *sregs);
	void setup_page_tables(kvm_sregs *sregs);
public:
	Bootloader(int vcpu_fd, uint8_t *guest_mem, uint32_t guest_mem_size, uint64_t entry_point, uint64_t start_stack);
	int run_long_mode();
	int run_protected_mode();
};
#endif