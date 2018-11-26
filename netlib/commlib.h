#ifndef COMMLIB_H_
#define COMMLIB_H_

#include <string.h>
#include <stdio.h>
#include <stdint.h>

int check_port_str(char *str);

int tcp_connect(const char *ip_addr, uint16_t port);
int tcp_start_server(const char * bind_addr, int port);
int tcp_accept_client(int srv_sock);
int recv_variable_message(int cl_sock, uint8_t* &buff, uint8_t &type);
int send_variable_message(int cl_sock, uint8_t type, uint8_t *buff, uint32_t bytes_count);

struct netfields {
private:
	netfields(const netfields& nfields) {}
	netfields operator=(const netfields& nfields){}

public:
	uint32_t count;
	uint8_t **data;
	uint32_t *size;


	netfields() {
		count = 0;
		data = 0;
		size = 0;
	}

	netfields(uint8_t f_count) {
		count = f_count;
		data = new uint8_t*[f_count];
		size = new uint32_t[f_count];
	}

	bool set_field(uint32_t field, uint8_t *field_data, uint32_t field_size) {
		if(field >= count)
			return false;

		size[field] = field_size;
		data[field] = new uint8_t[field_size];
		memcpy(data[field], field_data, field_size);

		return true;
	}

	bool get_field(uint32_t field, uint8_t *field_data, uint32_t expected_field_size) {
		if(field >= count)
			return false;

		if(size[field] != expected_field_size)
			return false;

		memcpy(field_data, data[field], size[field]);

		return true;
	}

	bool dealloc_fields() {
		for(int i=0; i<count; i++)
			delete[] data[i];
	}

	~netfields() {
		//printf("Destroying %p\n", data);
		fflush(0);
		delete[] data;
		delete[] size;
	}

};

int recv_field_message(int cl_sock, uint8_t &type, uint8_t &subtype, netfields* &nfields);
int send_field_message(int cl_sock, uint8_t type, uint8_t subtype, const netfields& nfields);

#endif