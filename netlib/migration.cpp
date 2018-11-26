#include "migration.h"
#include "commlib.h"
#include <cstdint>
#include <cstring>
#include <stdio.h>

int send_start_migr_message(int sock) {
	return send_variable_message(sock, TYPE_START_MIGRATION, NULL, 0);
}

int send_continue_migr_message(int sock) {
	return send_variable_message(sock, TYPE_CONTINUE_MIGRATION, NULL, 0);
}

int send_error_migr_message(int sock, char *error_str) {
	return send_variable_message(sock, TYPE_ERROR_MIGRATION, (uint8_t*)error_str, strlen(error_str)+1);
}

int send_commit_migr_message(int sock) {
	return send_variable_message(sock, TYPE_COMMIT_MIGRATION, NULL, 0);
}

int send_memory_end_migr_message(int sock) {
	netfields nfields(1);
	nfields.size[0] = 1;
	nfields.data[0] = new uint8_t[1];
	nfields.data[0][0] = 0;

	return send_field_message(sock, TYPE_DATA_MEMORY_END, 0, nfields);
}

int send_memory_message(int sock, uint32_t page_num, uint8_t *memory_page, uint32_t page_size) {
	netfields nfields(2);
	nfields.data[0] = memory_page;
	nfields.size[0] = page_size;
	nfields.set_field(1, (uint8_t*)&page_num, sizeof(page_num));

	return send_field_message(sock, TYPE_DATA_MEMORY, 0, nfields);
}

int receive_memory_message(int sock, uint32_t &page_num, uint8_t* &memory_page, uint8_t &type) {
	netfields *nfields;
	uint8_t subtype;

	int ret = recv_field_message(sock, type, subtype, nfields);
	if(ret < 0)
		return ret;

	if(type == TYPE_DATA_MEMORY_END)
		return 0;
	if(type != TYPE_DATA_MEMORY || nfields->count != 2)
		return -1;

	page_num = subtype;
	nfields->get_field(1, (uint8_t*)&page_num, sizeof(page_num));

	memory_page = nfields->data[0];
	//printf("received nfields->data = %p\n", nfields->data[0]);

	delete nfields;

	return ret;
}

int send_cpu_context_message(int sock, uint8_t* cpu_data, int size) {
	return send_variable_message(sock, TYPE_DATA_CPU_CONTEXT, cpu_data, size);
}

int send_io_context_message(int sock, uint8_t dev_type, uint8_t** device_field, uint16_t * device_field_size, int size) {
	// DA FINIRE, MANCA TIPO
	return send_variable_message(sock, TYPE_DATA_IO_CONTEXT, *device_field, size);
}

int wait_for_message(int sock, uint8_t message_type, uint8_t* &buff) {
	uint8_t type;
	int res = recv_variable_message(sock, buff, type);
	if(type != message_type)
		return -1;
	return res;
}