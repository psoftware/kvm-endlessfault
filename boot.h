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

void fill_segment_descriptor(uint64_t *dt, struct kvm_segment *seg);
void setup_protected_mode(int vcpu_fd , unsigned char *data_mem, uint64_t entry_point);
void setup_long_mode(int vcpu_fd , unsigned char *data_mem);
#endif