#ifndef CONSOLELOG_H
#define CONSOLELOG_H

#include <iostream>
#include <fstream>

//#define DEBUG_LOG

class ConsoleLog {
private:
	// private constructor because we want to apply Singleton design pattern
	ConsoleLog();

	// same thing for copy constructor
	ConsoleLog(ConsoleLog const&);

	// and for assignament operator
	void operator=(ConsoleLog const&);

	// stream instance
	std::ostream *logstream;

	pthread_mutex_t mutex;

public:
	// follow Singleton pattern
	static ConsoleLog* getInstance();

	// to change log file path
	void setFilePath(const char* filepath);

	// the instance will behave as a stream
	template<typename T>
	ConsoleLog& operator << (T&& x)
	{
		pthread_mutex_lock(&mutex);
		(*logstream) << std::forward<T>(x);
		pthread_mutex_unlock(&mutex);
		return *this;
	}

	template<typename T>
	ConsoleLog& operator << (const T& object)
	{
		pthread_mutex_lock(&mutex);
		(*logstream) << object;
		pthread_mutex_unlock(&mutex);
		return *this;
	}

	ConsoleLog& operator<<( std::ostream& (*pf)( std::ostream& ) )
	{
		pthread_mutex_lock(&mutex);
		(*logstream) << pf;
		pthread_mutex_unlock(&mutex);
		return *this;
	}

	ConsoleLog& operator<<( std::basic_ios<char>& (*pf)( std::basic_ios<char>& ))
	{
		pthread_mutex_lock(&mutex);
		(*logstream) << pf;
		pthread_mutex_unlock(&mutex);
		return *this;
	}
};

#endif