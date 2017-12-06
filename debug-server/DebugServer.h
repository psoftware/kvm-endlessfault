#ifndef DEBUG_SERVER_H
#define DEBUG_SERVER_H

#include <vector>
#include <thread>
#include "messages.h"
#include "net_wrapper.h"
#include "../backend/ConsoleLog.h"
#include <sys/ioctl.h>
#include <linux/kvm.h>

/**
 * Some useful information about 
 * the virtual machine is runnig
 *
 */ 
struct machine_info
{
	int vcpu_fd;
	void *guest_mem_ptr;
	uint64_t mem_size;
	machine_info( int vcpu_f, void *guest_m, uint64_t mem_s){ 
		vcpu_fd = vcpu_f;
		guest_mem_ptr = guest_m;
		mem_size = mem_s;
	}
};

/**
 * This class implements a TCP server
 * which provides debug services to clients.
 */
class DebugServer
{
	machine_info _mi;
	int serv_socket_;
	//std::vector<std::thread> peers_;
	std::thread main_thread_;
	void send_dump_mem(time_t timestamp, uint64_t start_addr, uint64_t end_addr);
	static void main_fun(int serv_sockt, machine_info mi);
	static void worker_fun(int sockt, machine_info);
public: 
	DebugServer(uint16_t port, int vcpu_fd, uint64_t mem_size, void *mem_ptr);
	~DebugServer();
	void start();
	static void send_dump_mem(int sockt, machine_info mi, uint64_t start_addr, uint64_t end_addr);
	static void send_info(int sockt, machine_info mi);
};

#endif