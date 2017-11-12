#include "ConsoleLog.h"
#include <fstream>
#include <iostream>

using namespace std;

ConsoleLog::ConsoleLog() : logstream(new std::ofstream("console.log", ios_base::out|ios_base::trunc))
{

}

ConsoleLog* ConsoleLog::getInstance()
{
	// seguiamo il design pattern Singleton
	static ConsoleLog unique_instance;
	return &unique_instance;
}