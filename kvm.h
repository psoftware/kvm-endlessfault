#ifndef KVM_H_
#define KVM_H_

// Migration functions
int get_vm_fd();
void start_source_migration(int vm_fd, const char* address, uint16_t port);

#endif