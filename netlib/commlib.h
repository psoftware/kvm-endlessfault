#ifndef COMMLIB_H_
#define COMMLIB_H_

#include <cstdint>

int check_port_str(char *str);

int tcp_connect(uint16_t port, char *ip_addr);
int recv_variable_message(int cl_sock, uint8_t *buff, uint8_t &type);
int send_variable_message(int cl_sock, uint8_t type, uint8_t *buff, uint32_t bytes_count);
int recv_field_message(int cl_sock, uint8_t **field_data, uint8_t* &bytes_count, uint8_t &type, uint8_t &subtype);
int send_field_message(int cl_sock, uint8_t type, uint8_t subtype, uint8_t **field_data, uint32_t *bytes_count, uint32_t field_count);

#endif