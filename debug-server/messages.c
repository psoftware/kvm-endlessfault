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
		case ACCPT_DUMP_MEM:
		case REQ_INFO:
			((simple_msg*)msg)->t = htonl(((req_dump_mem*)msg)->t);
			((simple_msg*)msg)->timestamp = htonl(((simple_msg*)msg)->timestamp);
			break;
		case SEND_INFO:
			((info_msg*)msg)->t = htonl(((info_msg*)msg)->t);
			((info_msg*)msg)->timestamp = htonl(((simple_msg*)msg)->timestamp);
			((info_msg*)msg)->mem_size = HTONLL(((info_msg*)msg)->mem_size);
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
		case ACCPT_DUMP_MEM:
		case REQ_INFO:
			((simple_msg*)msg)->timestamp = ntohl(((simple_msg*)msg)->timestamp);
			break;
		case SEND_INFO:
			((info_msg*)msg)->timestamp = ntohl(((simple_msg*)msg)->timestamp);
			((info_msg*)msg)->mem_size = NTOHLL(((info_msg*)msg)->mem_size);
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

void init_req_dump_mem(req_dump_mem *msg, uint64_t start_addr, uint64_t end_addr)
{
	msg->t = REQ_DUMP_MEM;
	msg->start_addr = start_addr;
	msg->end_addr = end_addr;
	msg->timestamp = time(NULL);
}

void init_simple_msg(simple_msg *msg, message_type t)
{
	msg->t = t;
	msg->timestamp = time(NULL);
}

void init_info_msg(info_msg *msg, uint64_t mem_size)
{
	msg->t = SEND_INFO;
	msg->mem_size = mem_size;
	msg->timestamp = time(NULL);
}