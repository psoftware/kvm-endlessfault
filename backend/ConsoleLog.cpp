#include "ConsoleLog.h"
using namespace std;

ConsoleLog::ConsoleLog() : logstream(0)
{
	pthread_mutex_init(&mutex, NULL);
}

ConsoleLog* ConsoleLog::getInstance()
{
	// seguiamo il design pattern Singleton
	static ConsoleLog unique_instance;
	return &unique_instance;
}

void ConsoleLog::setFilePath(const char* filepath)
{
	pthread_mutex_lock(&mutex);

	delete logstream;
	logstream = new std::ofstream(filepath, ios_base::out|ios_base::trunc);

	pthread_mutex_unlock(&mutex);
}