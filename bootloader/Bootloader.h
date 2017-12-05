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

#define BOOTLOADER_DIM 100


class Bootloader {
private:
	int vcpu_fd_;
	unsigned char *guest_mem_;
	uint64_t entry_point_;
	uint64_t protected_mode_start_stack_;
	void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg);
	void setup_protected_mode(struct kvm_sregs *sregs);
	void setup_page_tables(kvm_sregs *sregs);
public:
	Bootloader(int vcpu_fd, unsigned char *guest_mem, uint64_t entry_point, uint64_t protected_mode_start_stack);
	int run_long_mode();
	int run_protected_mode();
};
#endif