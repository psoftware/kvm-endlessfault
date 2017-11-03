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
bool ConsoleInput::is_shifted = false;

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

		// procediamo alla conversione
		bool new_shift = is_shifted;
		uint8_t newkeycode = ascii_to_keycode(newchar, new_shift);

		// dobbiamo emulare i keycode, i quali gestiscono l'evento shift in maniera particolare.
		// ogni qual volta shift viene premuto o rilasciatova mandato un determinato keycode,
		// diverso dal carattere digitato
		if(is_shifted != new_shift && new_shift == true)
			attached_keyboard->insert_keycode_event(0x2a);
		else if(is_shifted != new_shift && new_shift == false)
			attached_keyboard->insert_keycode_event(0xaa);
		is_shifted = new_shift;

		if(newkeycode)
		{
			cout << "console: inoltrato " << (unsigned int)newkeycode << endl;
			attached_keyboard->insert_keycode_event(newkeycode);
		}
	}
	// tecnicamente res non dovrebbe essere mai < 0
	while(res >= 0);
}

// tabella delle traduzioni
ConsoleInput::encode_table ConsoleInput::enc_t = {
		{	// tab
			0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
			0x0c,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
			0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
			0x26, 0x29, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
			0x33, 0x34, 0x35, 0x56,
			0x39, 0x1C, 0x0e, 0x01
		},
		{	// tamin
			'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
			'\'',
			'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
			'a', 's', 'd', 'f', 'g', 'h', 'j', 'k',
			'l', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm',
			',', '.', '-', '<',
			' ', '\n', '\b', 0x1B
		},
		{	// tabmai
			'!', '"', '@', '$', '%', '&', '/', '(', ')', '=',
			'?',
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
			'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',
			'L', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
			';', ':', '_', '>',
			' ', '\r', '\b', 0x1B
		}
	};

uint8_t ConsoleInput::ascii_to_keycode(uint8_t ascii_c, bool& shift)
{
	// scorriamo i caratteri non shiftati
	short pos = 0;
	while (pos < encode_table::MAX_CODE && enc_t.tabmin[pos] != ascii_c)
		pos++;

	// se non abbiamo trovato nulla cerchiamo tra quelli shiftati 
	// ma teniamone conto
	if(pos == encode_table::MAX_CODE)
	{
		pos = 0;
		while (pos < encode_table::MAX_CODE && enc_t.tabmai[pos] != ascii_c)
			pos++;

		// se non abbiamo trovato nulla neanche al secondo tentativo
		// restituiamo un carattere nullo
		if(pos == encode_table::MAX_CODE)
			return 0;

		// assegnamo qui lo shift per evitare di farlo nel caso il carattere
		// non è stato trovato
		shift = true;
	}
	else	// se abbiamo trovato il carattere nella prima tabella allora il carattere non è shiftato
		shift = false;

	return enc_t.tab[pos];
}