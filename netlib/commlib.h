#ifndef COMMLIB_H_
#define COMMLIB_H_

#include <cstdint>

int check_port_str(char *str);

int tcp_connect(const char *ip_addr, uint16_t port);
int tcp_start_server(const char * bind_addr, int port);
int recv_variable_message(int cl_sock, uint8_t *buff, uint8_t &type);
int send_variable_message(int cl_sock, uint8_t type, uint8_t *buff, uint32_t bytes_count);

struct netfields {
	uint32_t count;
	uint8_t **data;
	uint8_t *size;
};

int recv_field_message(int cl_sock, uint8_t &type, uint8_t &subtype, netfields& nfields);
int send_field_message(int cl_sock, uint8_t type, uint8_t subtype, const netfields& nfields);

#endif