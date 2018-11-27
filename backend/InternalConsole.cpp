#include <iostream>
#include <string>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <cstdint>

#include <signal.h>

#include "../kvm.h"
#include "InternalConsole.h"

using namespace std;


InternalConsole::InternalConsole(uint16_t port) {
	this->port = port;
}

void InternalConsole::start_thread() {
	pthread_create(&_thread, NULL, &InternalConsole::_mainThread, (void*)this);
}

void InternalConsole::stop_thread() {
	close(current_cl_sock);
	close(srv_sock);
}

bool InternalConsole::print_string(string str) {
	const char *cstr = str.c_str();
	bool res = print_string(cstr);
	delete[] cstr;
	return res;
}

bool InternalConsole::print_string(const char* str) {
	int resp_size = strlen(str)+1;
	int ret = send(current_cl_sock, (void*)str, resp_size, 0);
	if(ret == 0 || ret == -1 || ret < resp_size)
		return false;
	return true;
}

int InternalConsole::split_parameters(char *str, char** &substr) {
	unsigned int len = strlen(str);
	substr = new char*[len];

	substr[0] = &str[0];
	int substr_count = 1;
	for(int i=0; i<len-1; i++) {
		if(str[i] == ' ') {
			str[i] = '\0';
			substr[substr_count++] = &str[i+1];
		}
	}

	return substr_count;
}

bool InternalConsole::process_command(char *str_in, char *str_out) {
	char ** str_params;
	int str_params_count = split_parameters(str_in, str_params);

	if(str_params_count == 0) {
		strncpy(str_out, "unrecognized command\n", 1000);
		return true;
	}

	if(!strcmp(str_params[0], "exit"))
		return false;
	else if(!strcmp(str_params[0], "migrate"))
	{
		if(str_params_count != 3) {
			strncpy(str_out, "Usage: migrate <ip> <port>\n", 1000);
			return true;
		}

		int port = check_port_str(str_params[2]);
		if(!is_valid_IP(str_params[1]) || port == -1) {
			strncpy(str_out, "Invalid ip address or port\n", 1000);
			return true;
		}

		start_source_migration(get_vm_fd(), str_params[1], port);
		str_out[0] = '\0';
	}
	else
		strncpy(str_out, "unrecognized command\n", 1000);

	delete[] str_params;

	return true;
}

void* InternalConsole::_mainThread(void *param) {
	InternalConsole *This = (InternalConsole*)param;

	// Let main kvm thread to manage signals
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	This->srv_sock = tcp_start_server("0.0.0.0", This->port);
	if(This->srv_sock < 0)
		return NULL;

	while(true) {
		This->current_cl_sock = tcp_accept_client(This->srv_sock);
		int ret;

		char rec_str[1000];
		unsigned int rec_len = 0;

		char resp_str[1000];

		// send "vmm console>"
		strcpy(resp_str, "vmm console> ");
		if(!This->print_string(resp_str)) {
			close(This->current_cl_sock);
			continue;
		}

		// get one char at time (to simplify the code, can be optimized)
		while(true) {
			char c;
			ret = recv(This->current_cl_sock, (void*)&c, 1, MSG_WAITALL);

			if(ret < 0)
				break;

			if(c == '\n') {
				// ensure there is always at least a termination char
				rec_str[(rec_len == 0) ? 0 : rec_len-1] = '\0';

				// execute command
				bool requires_stop = !This->process_command(rec_str, resp_str);

				// ensure there is always at least a termination char
				resp_str[1000-1] = '\0';

				// send all resp_str buffer bytes
				if(!This->print_string(resp_str))
					break;

				// reset rec_str buffer
				rec_len = 0;

				// command wants to close console
				if(requires_stop) {
					close(This->current_cl_sock);
					break;
				}

				// send "vmm console>"
				strcpy(resp_str, "vmm console> ");
				if(!This->print_string(resp_str))
					break;
			} else if(rec_len <= 1000) {
				rec_str[rec_len] = c;
				rec_len++;
			}
		}

		close(This->current_cl_sock);
	}
}