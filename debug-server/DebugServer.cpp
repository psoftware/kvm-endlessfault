#include "DebugServer.h"

using namespace std;


extern ConsoleLog& logg;
DebugServer::DebugServer(uint16_t port, uint32_t mem_size, void *mem_ptr)
	: main_thread_()
{
	mem_size_ = mem_size;
	mem_ptr_ = mem_ptr;
	logg << "mem_size_=" << mem_size_ << endl;
	serv_socket_ = open_serverTCP(port);
	if( serv_socket_ == -1 )
		throw std::invalid_argument("Cannot open server");
	
}

void DebugServer::_main_fun(int serv_sockt, vector<thread> &peers, uint32_t mem_size, void *mem_ptr)
{
	ConnectionTCP c;
	while(true)
	{
		logg << "main fun" << endl;
		if( accept_serverTCP(serv_sockt, &c) != -1 )
		{
			logg << "nuovo peer" << endl;
			/*peers.push_back(*/thread t(&DebugServer::_worker_fun,c.socket,mem_size,mem_ptr)/*)*/;
			t.detach();
		}
	}
}

void DebugServer::_worker_fun(int sockt, uint32_t mem_size, void *mem_ptr)
{
	logg << "worker fun mem_size_:" << mem_size << endl;
	send_data(sockt, (char*)mem_ptr, mem_size);
}

void DebugServer::start()
{
	// move assignement
	main_thread_ = thread(&DebugServer::_main_fun,serv_socket_,std::ref(peers_),mem_size_,mem_ptr_);
	main_thread_.detach();
}