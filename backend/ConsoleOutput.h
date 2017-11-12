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
#include "../backend/ConsoleLog.h"

// logger globale
extern ConsoleLog& log;

#define COLS 80
#define ROWS 25
#define VIDEO_MEMORY_OFFSET 0x000B8000 // 0
#define REFRESH_TIME 0.06
#define CLEAR "\033[2J\033[1;1H"
#define STANDARD_BACKGROUND "\033[30;40m"

using namespace std;

class ConsoleOutput{

private:

	pthread_t _videoThread;

	//uint16_t* _videoMatrix;
	uint16_t _videoMatrix[ROWS*COLS];

	struct termios tty_attr_old;

	ConsoleOutput();
	ConsoleOutput(ConsoleOutput const&);
	void operator=(ConsoleOutput const&);

	uint16_t _min(uint16_t one, uint16_t two){ return (one<two)? one:two; }

	static void* _mainThread(void *This_par);
	
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

	void resetConsole();
};