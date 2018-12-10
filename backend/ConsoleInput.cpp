#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "ConsoleInput.h"

using namespace std;

ConsoleInput::ConsoleInput() : attached_keyboard(0), is_shifted(false)
{
	struct termios tty_attr;

	// we save current stdin operating mode in order to restore it on termination
	tcgetattr(STDIN_FILENO, &tty_attr_old);

	// we disable some stdin stream features (echoing, newline flush, etc)
	tcgetattr(STDIN_FILENO, &tty_attr);
	tty_attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty_attr.c_lflag &= ~(ICANON | ECHO | ECHONL | IEXTEN);
	tty_attr.c_cflag &= ~(CSIZE | PARENB);
	tty_attr.c_cflag |= CS8;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_attr);
}

ConsoleInput::~ConsoleInput()
{
	// restore the console to the initial state
	resetConsole();
}

ConsoleInput* ConsoleInput::getInstance()
{
	// follow Singleton pattern
	static ConsoleInput unique_instance;
	return &unique_instance;
}

void ConsoleInput::resetConsole()
{
	// reset the console state using the object saved in the constructor
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_attr_old);
}

bool ConsoleInput::attachKeyboard(keyboard *kb) {
	// only one keyboard can be connected and only once
	if(attached_keyboard != NULL)
		return false;

	attached_keyboard = kb;
}

bool ConsoleInput::startEventThread() {

	// we cannot continue if the keyboard is not connected or if we have started the thread
	if(_thread != 0 || attached_keyboard == NULL)
		return false;

	// console input managed by the thread
	pthread_create(&_thread, NULL, &ConsoleInput::_mainThread, this);
}

void* ConsoleInput::_mainThread(void *This_par)
{
	// mask all signals
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	ConsoleInput* This = (ConsoleInput*)This_par;

	char newchar;
	int res;

	// read method returns the ASCII code of the inserted char as soon as they are available
	do {
		res = read(0, &newchar, 1);

		// conversion
		bool new_shift = This->is_shifted;
		uint8_t newkeycode = ascii_to_keycode(newchar, new_shift);

		// When shift is pressed or released a certain keycode has to be sent which is different from the typed char
		if(This->is_shifted != new_shift && new_shift == true)
			This->attached_keyboard->insert_keycode_event(0x2a);
		else if(This->is_shifted != new_shift && new_shift == false)
			This->attached_keyboard->insert_keycode_event(0xaa);
		This->is_shifted = new_shift;

		// if the keycode is correctly converted then we send keydown event and then keyup event(in order to emulate press event)
		if(newkeycode)
		{
			#ifdef DEBUG_LOG
			logg << "console: forwarded (down) " << (unsigned int)newkeycode << endl;
			#endif
			This->attached_keyboard->insert_keycode_event(newkeycode);
			#ifdef DEBUG_LOG
			logg << "console: forwarded (up)" << (unsigned int)(newkeycode | 0x80) << endl;
			#endif
			This->attached_keyboard->insert_keycode_event(newkeycode | 0x80);
		}
	}
	//  res should be > 0
	while(res >= 0);
}

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
			' ', '\r', '\b', 0x1B
		},
		{	// tabmai
			'!', '"', '@', '$', '%', '&', '/', '(', ')', '=',
			'?',
			'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
			'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K',
			'L', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M',
			';', ':', '_', '>',
			' ', '\n', '\b', 0x1B
		}
	};

uint8_t ConsoleInput::ascii_to_keycode(uint8_t ascii_c, bool& shift)
{
	// non-shifted char
	short pos = 0;
	while (pos < encode_table::MAX_CODE && enc_t.tabmin[pos] != ascii_c)
		pos++;

	// if not we looking for non-shifted ones 
	if(pos == encode_table::MAX_CODE)
	{
		pos = 0;
		while (pos < encode_table::MAX_CODE && enc_t.tabmai[pos] != ascii_c)
			pos++;

		// if not we return null char
		if(pos == encode_table::MAX_CODE)
			return 0;

		// we assign here the shift to avoid doing it in case the char was not found
		shift = true;
	}
	else // if we find the char in the first tab then the char is not shifted
		shift = false;

	return enc_t.tab[pos];
}