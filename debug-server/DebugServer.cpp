#include "DebugServer.h"

using namespace std;

extern ConsoleLog& logg;
DebugServer::DebugServer(uint16_t port, int vcpu_fd, uint64_t mem_size, void *mem_ptr)
	: main_thread_(), _mi(vcpu_fd,mem_ptr,mem_size)
{
	serv_socket_ = open_serverTCP(port);
	if( serv_socket_ == -1 )
		throw std::invalid_argument("Cannot open server");	
}

DebugServer::~DebugServer()
{
	logg << "chiamato distruttore" << endl;
	main_thread_.~thread();
	close(serv_socket_);
}
void DebugServer::main_fun(int serv_sockt, /*vector<thread *> &peers,*/ machine_info mi)
{
	ConnectionTCP c;
	while( true )
	{
		if( accept_serverTCP(serv_sockt, &c) != -1 )
		{
			#ifdef DEBUG_LOG
			logg << "new client connected" << endl;
			#endif
			thread t(&DebugServer::worker_fun,c.socket,mi);
			t.detach();
		//	peers_.push_back(std::move(t));
		}
	}
}

void DebugServer::worker_fun(int sockt, machine_info mi)
{
//	logg << "worker fun mem_size_:" << std::dec << mi.mem_size << " addr:" <<std::hex << mi.guest_mem_ptr << endl;
	my_buffer my_buf;
	my_buf.size = 0;
	my_buf.buf = NULL;

	while( true ){
		if( !recv_data(sockt, &my_buf) ){
			logg << "client disconnected" << endl;
			return;
		}
		convert_to_host_order(my_buf.buf);
		switch( ((simple_msg*)my_buf.buf)->t )
		{
			case REQ_DUMP_MEM:
				send_dump_mem( sockt, mi, ((req_dump_mem*)my_buf.buf)->start_addr,
						   	   ((req_dump_mem*)my_buf.buf)->end_addr );				
				break;
			case REQ_INFO:
				send_info(sockt, mi);
			default:
				return;
		}
	}
}

void DebugServer::start()
{
	// move assignement
	//main_thread_ = thread(&DebugServer::_main_fun,serv_socket_,std::ref(peers_),_mi);
	main_thread_ = thread(&DebugServer::main_fun,serv_socket_,_mi);	
	main_thread_.detach();
}

void DebugServer::send_dump_mem(int sockt, machine_info mi, uint64_t start_addr, uint64_t end_addr)
{
	void *start_guest_dump = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + start_addr); 
	void *end_guest_dump = reinterpret_cast<void *>(reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + end_addr);
	uint64_t dump_size = reinterpret_cast<uint64_t>(end_guest_dump) - reinterpret_cast<uint64_t>(start_guest_dump);

	#ifdef DEBUG_LOG
	logg << "client request memory dump start_addr:"<< start_addr << "end_addr:" << end_addr <<endl;
	#endif

	if( reinterpret_cast<uint64_t>(start_guest_dump) > reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + mi.mem_size ||
	    reinterpret_cast<uint64_t>(end_guest_dump) > reinterpret_cast<uint64_t>(mi.guest_mem_ptr) + mi.mem_size )
	{
		simple_msg m;
		init_simple_msg(&m,GENERIC_ERR);
		convert_to_network_order(&m);
	} else 
		send_data(sockt, (char*)start_guest_dump, dump_size);
}

void DebugServer::send_info(int sockt, machine_info mi)
{
	info_msg msg;
	kvm_regs regs;

	#ifdef DEBUG_LOG
	logg << "client request info about vm" <<endl;
	#endif

	init_info_msg(&msg, mi.mem_size);
	convert_to_network_order(&msg);
	int r = send_data(sockt, (char*)&msg, sizeof(msg));

	if (ioctl(mi.vcpu_fd, KVM_GET_REGS, &regs) < 0) {
		logg << "trace_user_program KVM_GET_REGS error: " << strerror(errno) << endl;
		return;
	}

	kvm_sregs sregs;
	if (ioctl(mi.vcpu_fd, KVM_GET_SREGS, &sregs) < 0) {
		logg << "trace_user_program KVM_GET_SREGS error: " << strerror(errno) << endl;
		return;
	}

	send_data(sockt, (char*)&regs, sizeof(regs));
	send_data(sockt, (char*)&sregs, sizeof(sregs));
}