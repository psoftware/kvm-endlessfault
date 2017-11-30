#include "messages.h"

#include <sys/param.h>

// conversioni host order <-------> network order per interi su 64bit
#define HTONLL(x) ((1==htonl(1)) ? (x) : (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
#define NTOHLL(x) ((1==ntohl(1)) ? (x) : (((uint64_t)ntohl((x) & 0xFFFFFFFFUL)) << 32) | ntohl((uint32_t)((x) >> 32)))

void convert_to_network_order(void* msg)
{
	message_type* m_t = (message_type*)msg;
	switch( *m_t ){
		case WELCOME_MESS:
		case GENERIC_ERR:
		case SERVER_QUIT:
		case REQ_INFO:
		case ACCPT_DUMP_MEM:
			((simple_mess*)msg)->t = htonl(((req_dump_mem*)msg)->t);
			((simple_mess*)msg)->timestamp = htonl(((simple_mess*)msg)->timestamp);
			break;
		case REQ_DUMP_MEM:
			((req_dump_mem*)msg)->t = htonl(((req_dump_mem*)msg)->t);
			((req_dump_mem*)msg)->timestamp = htonl(((req_dump_mem*)msg)->timestamp);
			((req_dump_mem*)msg)->start_addr = HTONLL(((req_dump_mem*)msg)->start_addr);
			((req_dump_mem*)msg)->end_addr = HTONLL(((req_dump_mem*)msg)->end_addr);
			break;
		default:
			break;
	}
}

void convert_to_host_order(void* msg)
{
	/*il campo type dei messaggi viene convertito
	  nel formato host order prima dello switch */
	message_type* m_t = (message_type*)msg;
	*m_t = ntohl(*m_t);

	switch( *m_t ){
		case WELCOME_MESS:
		case GENERIC_ERR:
		case SERVER_QUIT:
		case REQ_INFO:
		case ACCPT_DUMP_MEM:
			((simple_mess*)msg)->timestamp = ntohl(((simple_mess*)msg)->timestamp);
			break;
		
		case REQ_DUMP_MEM:
			((req_dump_mem*)msg)->timestamp = ntohl(((req_dump_mem*)msg)->timestamp);
			((req_dump_mem*)msg)->start_addr = NTOHLL(((req_dump_mem*)msg)->start_addr);
			((req_dump_mem*)msg)->end_addr = NTOHLL(((req_dump_mem*)msg)->end_addr);
			break;
		default:
			break;
	}
}

void init_req_dump_mem(req_dump_mem *mess, uint64_t start_addr, uint64_t end_addr)
{
	mess->t = REQ_DUMP_MEM;
	mess->start_addr = start_addr;
	mess->end_addr = end_addr;
	mess->timestamp = time(NULL);
}

void init_simple_mess(simple_mess *mess, message_type t)
{
	mess->t = t;
	mess->timestamp = time(NULL);
}

void init_send_info_mess(send_info *mess, uint64_t mem_size)
{
	mess->t = SEND_INFO;
	mess->mem_size = mem_size;
	mess->timestamp = time(NULL);
}