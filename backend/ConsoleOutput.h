#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <pthread.h>
#include <string>
#include "unistd.h"
#include "linux/kd.h"
#include "termios.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../frontend/vgaController.h"

#define COLS 80
#define ROWS 25
#define REFRESH_TIME 0.06
#define BLINK_TIME  1
#define CLEAR "\033[2J"
#define STANDARD_BACKGROUND "\033[30;40m"
#define CURSOR_START "\033[0;0H"
#define HIDE_CURSOR "\033[?25l"

using namespace std;

class ConsoleOutput{

private:

	pthread_t _videoThread;
	pthread_t _cursorBlink;

	VGAController* _vga;
	bool _isBlinking;
	pthread_mutex_t _cursorMutex;

	uint16_t* _videoMatrix;

	struct termios tty_attr_old;

	ConsoleOutput();
	ConsoleOutput(ConsoleOutput const&);
	void operator=(ConsoleOutput const&);

	uint16_t _min(uint16_t one, uint16_t two){ return (one<two)? one:two; }

	static void* _mainThread(void *This_par);
	static void* _blinkThread(void* param);
	
	string _getBackgroundColor(uint32_t code);
	string _getTextColor(uint32_t code);

	struct colorTable{

		static const int BACKGROUND_SIZE = 8;
		static const int TEXT_SIZE = 16;
		string supportedTextColor[TEXT_SIZE];
		string supportedBackgroundColor[BACKGROUND_SIZE];

	};

	static colorTable color_t;

public:

	~ConsoleOutput();

	static ConsoleOutput* getInstance();

	bool startThread();

	bool attachVGA(VGAController* v);

	void resetConsole();
};

#endif