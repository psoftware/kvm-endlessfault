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

void DebugServer::_main_fun(int serv_sockt, /*vector<thread *> &peers,*/ machine_info mi)
{
	ConnectionTCP c;
	while(true)
	{
		logg << "main fun" << endl;
		if( accept_serverTCP(serv_sockt, &c) != -1 )
		{
			logg << "nuovo peer" << endl;
			thread t(&DebugServer::_worker_fun,c.socket,mi);
			t.detach();
		//	peers_.push_back(std::move(t));
		}
	}
}

void DebugServer::_worker_fun(int sockt, machine_info mi)
{
	logg << "worker fun mem_size_:" << std::dec << mi.mem_size << " addr:" <<std::hex << mi.guest_mem_ptr << endl;
	my_buffer my_buf;
	my_buf.size = 0;
	my_buf.buf = NULL;
	bool logged = true;

	while( true ){
		//logg << "recv_data su " << std::dec << sockt << endl;
		if( !recv_data(sockt, &my_buf) ){
			logg << "client disconnected" << endl;
			return;
		}
		//logg << "convert_to_host_order" << endl;
		convert_to_host_order(my_buf.buf);
		//logg << "switch " << (int)(((simple_mess*)my_buf.buf)->t) << endl;
		switch( ((simple_mess*)my_buf.buf)->t )
		{
			case WELCOME_MESS:
				logg << "new client logged" << endl;
				logged = true;
				break;
			case REQ_DUMP_MEM:
				_send_dump_mem( sockt, mi, ((req_dump_mem*)my_buf.buf)->timestamp,
							   ((req_dump_mem*)my_buf.buf)->start_addr,
							   ((req_dump_mem*)my_buf.buf)->end_addr );
				break;
			default:
				return;
		}
	}
}

void DebugServer::start()
{
	// move assignement
	//main_thread_ = thread(&DebugServer::_main_fun,serv_socket_,std::ref(peers_),_mi);
	main_thread_ = thread(&DebugServer::_main_fun,serv_socket_,_mi);	
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