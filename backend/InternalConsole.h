#include <iostream>
#include <string>
#include <pthread.h>

#include "../netlib/commlib.h"

using namespace std;

class InternalConsole {
	pthread_t _thread;

	int srv_sock;
	int current_cl_sock;
public:
	InternalConsole();
	InternalConsole start_thread();

	bool print_string(string str);
	bool print_string(const char* str);

private:
	bool process_command(const char *str_in, char *str_out);
	static void* _mainThread(void *param);
};