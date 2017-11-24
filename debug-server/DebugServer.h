#ifndef DEBUG_SERVER_H
#define DEBUG_SERVER_H

#include <vector>
#include <thread>
#include "net_wrapper.h"
#include "../backend/ConsoleLog.h"

class DebugServer
{
	void *mem_ptr_;
	uint32_t mem_size_;
	int serv_socket_;
	std::vector<std::thread> peers_;
	std::thread main_thread_;
public: 
	DebugServer(uint16_t port, uint32_t mem_size, void *mem_ptr);
	static void _main_fun(int serv_sockt, DebugServer *d);
	static void _worker_fun(int sockt, DebugServer *d);
	void start();
};

#endif