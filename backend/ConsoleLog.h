#ifndef CONSOLELOG_H
#define CONSOLELOG_H

#include <iostream>
#include <fstream>

#define DEBUG_LOG

class ConsoleLog {
private:
	// il costruttore è privato perchè vogliamo seguire il design pattern Singleton
	ConsoleLog();

	// stessa cosa per il construttore di copia
	ConsoleLog(ConsoleLog const&);

	// e per l'operatore di assegnamento
	void operator=(ConsoleLog const&);

	// istanza dello stream
	std::ostream *logstream;

	// vogliamo un'implementazione thread safe di questa libreria
	pthread_mutex_t mutex;

public:
	// seguiamo il design pattern Singleton
	static ConsoleLog* getInstance();

	// possiamo cambiare il logfile in qualsiasi momento
	void setFilePath(const char* filepath);

	// la nostra istanza si comporterà similmente ad uno stream
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