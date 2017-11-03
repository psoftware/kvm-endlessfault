#ifndef CONSOLEINPUT_H
#define CONSOLEINPUT_H

#include "../frontend/keyboard.h"
#include <pthread.h>

class ConsoleInput {
private:
	struct encode_table { //
		static const int MAX_CODE = 46;
		uint8_t tab[MAX_CODE];
		uint8_t tabmin[MAX_CODE];
		uint8_t tabmai[MAX_CODE];
	};

	// seguiamo il design pattern Singleton
	static ConsoleInput *unique_instance;

	// teniamo da parte lo stato della console prima di effettuare modifiche per
	// ripristinarla su resetConsole()
	static struct termios tty_attr_old;

	// alla console deve essere collegata una tastiera (emulata) per l'inoltro degli input
	static keyboard *attached_keyboard;

	// ci servirà un thread per la gestione degli input perchè useremo read bloccanti
	static pthread_t _thread;

	// tabella per la codifica dei caratteri in keycode
	static encode_table enc_t;

	// dobbiamo ricordarci se abbiamo ricevuto un carattere shiftato
	static bool is_shifted;

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

	// dobbiamo convertire le codifiche ASCII dei caratteri ricevuti in keycode
	static uint8_t ascii_to_keycode(uint8_t ascii_c, bool& shift);
};

#endif