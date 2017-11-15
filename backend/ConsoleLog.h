#ifndef CONSOLELOG_H
#define CONSOLELOG_H

#include <iostream>
#include <fstream>

#define SUPPRESS_DEBUG


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

public:
	// seguiamo il design pattern Singleton
	static ConsoleLog* getInstance();

	// possiamo cambiare il logfile in qualsiasi momento
	void setFilePath(const char* filepath);

	// la nostra istanza si comporterà similmente ad uno stream
	template<typename T>
	ConsoleLog& operator << (T&& x)
	{
		(*logstream) << std::forward<T>(x);
		return *this;
	}

	template<typename T>
	ConsoleLog& operator << (const T& object)
	{
		(*logstream) << object;
		return *this;
	}

	ConsoleLog& operator<<( std::ostream& (*pf)( std::ostream& ) )
	{
	    (*logstream) << pf;
		return *this;
	}

	ConsoleLog& operator<<( std::basic_ios<char>& (*pf)( std::basic_ios<char>& ))
	{
	    (*logstream) << pf;
		return *this;
	}
};

#endif