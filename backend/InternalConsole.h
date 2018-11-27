#include <iostream>
#include <string>
#include <pthread.h>

#include "../netlib/commlib.h"

using namespace std;

class InternalConsole {
	pthread_t _thread;

	uint16_t port;

	int srv_sock;
	int current_cl_sock;
public:
	InternalConsole(uint16_t port);
	void start_thread();
	void stop_thread();

	bool print_string(string str);
	bool print_string(const char* str);

private:
	int split_parameters(char *str, char** &substr);
	bool process_command(char *str_in, char *str_out);
	static void* _mainThread(void *param);
};