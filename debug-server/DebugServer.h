#ifndef DEBUG_SERVER_H
#define DEBUG_SERVER_H

#include <vector>
#include <thread>
#include "messages.h"
#include "net_wrapper.h"
#include "../backend/ConsoleLog.h"

struct machine_info
{
	void *guest_mem_ptr;
	uint64_t mem_size;
	machine_info( void *guest_m, uint64_t mem_s){ 
		guest_mem_ptr = guest_m;
		mem_size = mem_s;
	}
};

class DebugServer
{
	machine_info _mi;
	int serv_socket_;
	std::vector<std::thread *> peers_;
	std::thread main_thread_;
	void send_dump_mem(time_t timestamp, uint64_t start_addr, uint64_t end_addr);
public: 
	DebugServer(uint16_t port, uint64_t mem_size, void *mem_ptr);
	static void _main_fun(int serv_sockt, std::vector<std::thread *> &peers, machine_info mi);
	static void _worker_fun(int sockt, machine_info);
	static void _send_dump_mem(int sockt, machine_info mi, time_t timestamp, uint64_t start_addr, uint64_t end_addr);
	void start();
};

#endif