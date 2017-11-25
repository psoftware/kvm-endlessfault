#include "DebugServer.h"

using namespace std;

extern ConsoleLog& logg;
DebugServer::DebugServer(uint16_t port, uint64_t mem_size, void *mem_ptr)
	: main_thread_(), _mi(mem_ptr,mem_size)
{
	logg << "mem_size_=" << _mi.mem_size << endl;
	serv_socket_ = open_serverTCP(port);
	if( serv_socket_ == -1 )
		throw std::invalid_argument("Cannot open server");	
}

void DebugServer::_main_fun(int serv_sockt, vector<thread *> &peers, machine_info mi)
{
	ConnectionTCP c;
	while(true)
	{
		logg << "main fun" << endl;
		if( accept_serverTCP(serv_sockt, &c) != -1 )
		{
			logg << "nuovo peer" << endl;
			peers.push_back(new thread(&DebugServer::_worker_fun,c.socket,mi));
		}
	}
}

void DebugServer::_worker_fun(int sockt, machine_info mi)
{
	logg << "worker fun mem_size_:" << mi.mem_size << endl;
	my_buffer my_buf;
	while( true ){
		recv_data(sockt, &my_buf);
		convert_to_host_order(my_buf.buf);
		switch( ((simple_mess*)my_buf.buf)->t )
		{
			case WELCOME_MESS:
				logg << "new client logged" << endl;
				break;
			case REQ_DUMP_MEM:
				_send_dump_mem( sockt, mi, ((req_dump_mem*)my_buf.buf)->timestamp,
							   ((req_dump_mem*)my_buf.buf)->start_addr,
							   ((req_dump_mem*)my_buf.buf)->end_addr );
				break;
		}
	}
}

void DebugServer::start()
{
	// move assignement
	main_thread_ = thread(&DebugServer::_main_fun,serv_socket_,std::ref(peers_),_mi);
	main_thread_.detach();
}

void DebugServer::_send_dump_mem(int sockt, machine_info mi, time_t timestamp, uint64_t start_addr, uint64_t end_addr)
{
	void *start_guest_dump = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + start_addr); 
	void *end_guest_dump = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + end_addr);
	uint64_t dump_size = reinterpret_cast<uint64_t>(end_guest_dump) - reinterpret_cast<uint64_t>(start_guest_dump);

	if( reinterpret_cast<uint64_t>(start_guest_dump) > reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + mi.mem_size ||
	    reinterpret_cast<uint64_t>(end_guest_dump) > reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + mi.mem_size )
	{
		simple_mess m;
		init_simple_mess(&m,GENERIC_ERR);
	} else 
		send_data(sockt, (char*)start_guest_dump, dump_size);
}