#ifndef CONSOLEINPUT_H
#define CONSOLEINPUT_H

#include "../frontend/keyboard.h"
#include <pthread.h>

class ConsoleInput {
private:
	// seguiamo il design pattern Singleton
	static ConsoleInput *unique_instance;

	// teniamo da parte lo stato della console prima di effettuare modifiche per
	// ripristinarla su resetConsole()
	static struct termios tty_attr_old;

	// alla console deve essere collegata una tastiera (emulata) per l'inoltro degli input
	static keyboard *attached_keyboard;

	// ci servirà un thread per la gestione degli input perchè useremo read bloccanti
	static pthread_t _thread;

	// il costruttore è privato perchè vogliamo seguire il design pattern Singleton
	ConsoleInput();

public:
	~ConsoleInput();

	// seguiamo il design pattern Singleton
	static ConsoleInput* getInstance();

	// metodo per collegare una tastiera alla console
	static bool attachKeyboard(keyboard *kb);

	// metodo per l'avvio del thread di gestione degli eventi di input
	static bool startEventThread();

	// questo metodo ci serve per resettare lo stato della console a fine esecuzione dell'applicazione
	static void resetConsole();

private:
	// starting point del thread che si occuperà di gestire l'input della console
	static void* _mainThread(void *nullparam);

};

#endif