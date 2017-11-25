#ifndef MESSAGES_H
#define MESSAGES_H
#include <time.h> 
#include <arpa/inet.h>
typedef enum 
{ 
	WELCOME_MESS,
	GENERIC_ERR,
	REQ_DUMP_MEM,
	ACCPT_DUMP_MEM,
	SERVER_QUIT,
} message_type;

typedef struct simple_mess_t
{
	message_type t;
	time_t timestamp;
}__attribute__((packed)) simple_mess;

typedef struct req_dump_mem_t
{
	message_type t;
	time_t timestamp;
	uint64_t start_addr;
	uint64_t end_addr;
}__attribute__((packed)) req_dump_mem;

#ifdef __cplusplus
extern "C" {
#endif
/*******************************************
* Riconosce automaticamente il messaggio e
* lo converte nel network order
********************************************/
void convert_to_network_order(void* msg);

/*******************************************
* Riconosce automaticamente il messaggio e
* lo converte nell'host order
********************************************/
void convert_to_host_order(void* msg);

void init_generic_err(simple_mess *mess);
void init_req_dump_mem(req_dump_mem *mess, uint64_t start_addr, uint64_t end_addr);
#ifdef __cplusplus
}
#endif

#endif
