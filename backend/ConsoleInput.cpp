#include "unistd.h"
#include "linux/kd.h"
#include "termios.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ConsoleInput.h"

using namespace std;

// variabili statiche (vedi commenti nel file header)
ConsoleInput* ConsoleInput::unique_instance = NULL;
termios ConsoleInput::tty_attr_old;
keyboard* ConsoleInput::attached_keyboard = NULL;
pthread_t ConsoleInput::_thread;

ConsoleInput::ConsoleInput()
{
	struct termios tty_attr;

	// salviamo la modalità di funzionamento attuale di stdin per ripristinarla alla chiusura
	tcgetattr(STDIN_FILENO, &tty_attr_old);

	// disabilitiamo alcune funzionalità dello stream stdin (echoing, flush su newline, etc)
	tcgetattr(STDIN_FILENO, &tty_attr);
	tty_attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty_attr.c_lflag &= ~(ICANON | ECHO | ECHONL | IEXTEN);
	tty_attr.c_cflag &= ~(CSIZE | PARENB);
	tty_attr.c_cflag |= CS8;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_attr);
}

ConsoleInput::~ConsoleInput()
{
	// la console va ripristinata
	resetConsole();

	// eliminiamo l'istanza globale
	delete unique_instance;
}

ConsoleInput* ConsoleInput::getInstance()
{
	// seguiamo il design pattern Singleton
	if(unique_instance == NULL)
		unique_instance = new ConsoleInput();

	return unique_instance;
}

void ConsoleInput::resetConsole()
{
	// resettiamo lo stato della console utilizzando l'oggetto salvatoci nel costruttore
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_attr_old);
}

bool ConsoleInput::attachKeyboard(keyboard *kb) {
	// può essere collegata solo una tastiera e solo per una volta
	if(attached_keyboard != NULL)
		return false;

	attached_keyboard = kb;
}

bool ConsoleInput::startEventThread() {
	// non possiamo continuare se la tastiera non è stata collegata oppure se abbiamo già
	// avviato il thread
	if(_thread != 0 || attached_keyboard == NULL)
		return false;

	// lasciamo al thread il compito di ottenere l'input dalla console
	pthread_create(&_thread, NULL, &ConsoleInput::_mainThread, NULL);
}

void* ConsoleInput::_mainThread(void *nullparam)
{
	char newchar;
	int res;

	// la read ci restituisce la codifica in ASCII dei caratteri
	// inseriti nella console, appena questi sono disponibili
	do {
		res = read(0, &newchar, 1);
		cout << "console: inoltrato " << (unsigned int)newchar << endl;
		attached_keyboard->insert_keycode_event(newchar);
	}
	// tecnicamente res non dovrebbe essere mai < 0
	while(res >= 0);
}