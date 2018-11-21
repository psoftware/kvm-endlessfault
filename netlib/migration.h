#ifndef MIGRATION_H_
#define MIGRATION_H_

#include <cstdint>

#include "migration.h"
#include "commlib.h"

/*********  Packets *********
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
|dim|255|error string|
*/

// ---> From source
#define TYPE_START_MIGRATION 0
#define TYPE_COMMIT_MIGRATION 2

#define TYPE_DATA_MEMORY 10
#define TYPE_DATA_MEMORY_END 11
#define TYPE_DATA_CPU_CONTEXT 12
#define TYPE_DATA_IO_CONTEXT 13

// <--- From Destination
#define TYPE_CONTINUE_MIGRATION 1

// <--> From Both
#define TYPE_ERROR_MIGRATION 255

// Device types
#define IO_TYPE_KEYBOARD 0
#define IO_TYPE_VGACONTROLLER 1

int send_start_migr_message(int sock);
int send_continue_migr_message(int sock);
int send_error_migr_message(int sock, char *error_str);
int send_commit_migr_message(int sock);
int send_memory_end_migr_message(int sock);
int send_memory_message(int sock, uint32_t page_num, uint8_t *memory_page, uint32_t page_size);
int send_cpu_context_message(int sock, uint8_t* cpu_data, int size);
int send_io_context_message(int sock, uint8_t dev_type, uint8_t** device_data, int size);

int wait_for_message(int sock, uint8_t message_type, uint8_t* &buff);

#endif