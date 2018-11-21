#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>

#include <ctype.h>

#include <errno.h>
#include <cstdint>

#include "commlib.h"

int tcp_connect(const char *ip_addr, uint16_t port) {
	int sock_client = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip_addr, &srv_addr.sin_addr);

	if(connect(sock_client, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
	{
		perror("commlib: connect fail");
		return -1;
	}

	//printf("\nConnected to server %s:%d) \n\n", argv[1], porta);
	return sock_client;
}

int tcp_start_server(const char * bind_addr, int port)
{
	int ret_sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in my_addr;
	memset(&my_addr, 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	inet_pton(AF_INET, bind_addr, &my_addr.sin_addr);

	if(bind(ret_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1)
	{
		perror("Bind fallita");
		return -1;
	}
	if(listen(ret_sock, 10) == -1)
	{
		perror("Listen fallita");
		return -1;
	}

	return ret_sock;
}

int tcp_accept_client(int srv_sock) {
	struct sockaddr_in cl_addr;
	int my_len = sizeof(cl_addr);
	return accept(srv_sock, (struct sockaddr*)&cl_addr, (socklen_t*)&my_len);
}


int recv_variable_message(int cl_sock, uint8_t* &buff, uint8_t &type)
{
	unsigned int bytes_count;

	// receive the type of message
	int ret = recv(cl_sock, (void*)&type, sizeof(uint8_t), MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;

	// receive the number of byte to read next
	ret = recv(cl_sock, (void*)&bytes_count, sizeof(unsigned int), MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;

	bytes_count=ntohl(bytes_count);

	if(bytes_count == 0) {
		printf("empty message\n");
		return bytes_count;
	}

	// allocate buffer
	buff = new uint8_t[bytes_count];

	// receive all the bytes_count bytes
	ret = recv(cl_sock, (void*)buff, bytes_count, MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;
	if(ret < bytes_count)
	{
		printf("recv_variable_string: Byte ricevuti (%d) minori di quelli previsti!\n", ret);
		return -1;
	}

	return bytes_count;
}

int send_variable_message(int cl_sock, uint8_t type, uint8_t *buff, uint32_t bytes_count)
{
	// send the message type
	int ret = send(cl_sock, &type, sizeof(uint8_t), 0);
	if(ret == 0 || ret == -1)
		return ret;

	// send the number of byte to read after this integer
	int net_bytes_count = htonl(bytes_count);
	ret = send(cl_sock, (unsigned int*)&net_bytes_count, sizeof(unsigned int), 0);
	if(ret == 0 || ret == -1)
		return ret;

	// if buff is empty don't continue
	if(bytes_count == 0)
		return ret;

	// send all buffer bytes
	ret = send(cl_sock, (void*)buff, bytes_count, 0);
	if(ret == 0 || ret == -1)
		return ret;
	if(ret < (int)bytes_count)
	{
		printf("send_variable_message: Received (%d) less bytes then sent ones!\n", ret);
		return -1;
	}
	return ret;
}

int recv_field_message(int cl_sock, uint8_t &type, uint8_t &subtype, netfields& nfields)
{
	int ret;

	// receive the type of message
	ret = recv(cl_sock, (void*)&type, sizeof(uint8_t), MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;

	// receive the subtype of message
	ret = recv(cl_sock, (void*)&subtype, sizeof(uint8_t), MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;


	// receive the number of fields to read
	ret = recv(cl_sock, (void*)&nfields.count, sizeof(uint32_t), MSG_WAITALL);
	if(ret == 0 || ret == -1)
		return ret;

	// allocate nfields.data and nfields.size
	nfields = netfields(nfields.count);

	for(uint32_t field=0; field<nfields.count; field++) {
		// receive the field size
		uint32_t field_len;
		ret = recv(cl_sock, (void*)&field_len, sizeof(uint32_t), MSG_WAITALL);
		if(ret == 0 || ret == -1)
			return ret;

		// save field len
		nfields.size[field] = field_len;

		// allocate field
		nfields.data[field] = new uint8_t[field_len];

		// receive field
		ret = recv(cl_sock, (void*)nfields.data[field], nfields.size[field], MSG_WAITALL);
		if(ret == 0 || ret == -1)
			return ret;
		if(ret < field_len)
		{
			printf("recv_variable_string: Byte ricevuti (%d) minori di quelli previsti!\n", ret);
			return -1;
		}

	}

	return ret;
}

int send_field_message(int cl_sock, uint8_t type, uint8_t subtype, const netfields& nfields)
{
	int ret;

	// send the message type
	ret = send(cl_sock, &type, sizeof(uint8_t), 0);
	if(ret == 0 || ret == -1)
		return ret;

	// send the message subtype
	ret = send(cl_sock, &subtype, sizeof(uint8_t), 0);
	if(ret == 0 || ret == -1)
		return ret;

	// send the number of array elements to read
	ret = send(cl_sock, (void*)(&nfields.count), sizeof(nfields.count), 0);
	if(ret == 0 || ret == -1)
		return ret;

	// send all fields
	for(uint32_t field=0; field<nfields.count; field++)
	{
		// send the field size
		ret = send(cl_sock, (void*)&nfields.size[field], sizeof(uint32_t), 0);
		if(ret == 0 || ret == -1)
			return ret;

		ret = send(cl_sock, (void*)nfields.data[field], nfields.size[field], 0);
		if(ret == 0 || ret == -1)
			return ret;
		if(ret < (int)nfields.size[field])
		{
			printf("send_variable_message: Received (%d) less bytes then sent ones!\n", ret);
			return -1;
		}
	}
	return ret;
}

int check_port_str(char * str)
{
	int i;
	for(i=0; i<strlen(str); i++)
		if(!isdigit(str[i]))
			return -1;

	int portnum = atoi(str);
	if(portnum < 0 || portnum > 65535)
		return -1;

	return portnum;
}
