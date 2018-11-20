#ifndef MIGRATION_H_
#define MIGRATION_H_

#include <cstdint>

#include "migration.h"
#include "commlib.h"

/*
|dim (byte)|tipo messaggio|info....|
   8 byte       1 byte       dim

------>
Start Migration
|dim|0|dim ram|

Commit Migration
|dim|2|

Memory
|dim|10|num pagina|pagina 4k|

CPU Context
|dim|11|contesto...|

IO Context
|dim|12|dev id|field count|field 1 size|field 1|field 2 size|field 2|...|field n size|field n|

<------
Continue
|dim|1|

<----->
Generic Error
|dim|3|error string|
*/

// ---> From source
#define TYPE_START_MIGRATION 0
#define TYPE_COMMIT_MIGRATION 2

#define TYPE_DATA_MEMORY 10
#define TYPE_DATA_CPU_CONTEXT 11
#define TYPE_DATA_IO_CONTEXT 12

// <--- From Destination
#define TYPE_CONTINUE_MIGRATION 1

// <--> From Both
#define TYPE_ERROR_MIGRATION 3

int send_start_migr_message(int cl_sock);
int send_continue_migr_message(int cl_sock);
int send_error_migr_message(int cl_sock, char *error_str);
int send_commit_migr_message(int cl_sock);
int send_memory_message(int cl_sock, uint32_t page_num, uint64_t *memory_page, uint16_t page_size);
int send_cpu_context_message(int cl_sock, uint8_t* cpu_data, int size);
int send_io_context_message(int cl_sock, uint8_t dev_type, uint8_t** device_data, int size);

#endif