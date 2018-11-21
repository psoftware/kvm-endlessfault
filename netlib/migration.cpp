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
	return send_variable_message(sock, TYPE_DATA_MEMORY_END, NULL, 0);
}

int send_memory_message(int sock, uint32_t page_num, uint8_t *memory_page, uint32_t page_size) {
	return send_variable_message(sock, TYPE_DATA_MEMORY, memory_page, page_size);
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