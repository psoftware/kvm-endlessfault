#include "ConsoleLog.h"
#include <fstream>
#include <iostream>

using namespace std;

ConsoleLog::ConsoleLog() : logstream(0)
{

}

ConsoleLog* ConsoleLog::getInstance()
{
	// seguiamo il design pattern Singleton
	static ConsoleLog unique_instance;
	return &unique_instance;
}

void ConsoleLog::setFilePath(const char* filepath)
{
	delete logstream;
	logstream = new std::ofstream(filepath, ios_base::out|ios_base::trunc);
}