#include "ConsoleOutput.h"

ConsoleOutput::ConsoleOutput(){

	tcgetattr(STDOUT_FILENO, &tty_attr_old);
	pthread_mutex_init(&_cursorMutex, NULL);
	_isBlinking = true;
	
}

ConsoleOutput::~ConsoleOutput()
{
	// la console va ripristinata
	resetConsole();
}


void ConsoleOutput::resetConsole()
{
	// resettiamo lo stato della console utilizzando l'oggetto salvatoci nel costruttore
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &tty_attr_old);

	// lanciamo un reset console
	//printf("\033c");
	cout<<RESET_CONSOLE;
}


ConsoleOutput* ConsoleOutput::getInstance()
{
	// seguiamo il design pattern Singleton
	static ConsoleOutput unique_instance;
	return &unique_instance;
}



bool ConsoleOutput::attachVGA(VGAController* v){

	if(_vga != NULL)
		return false;

	_vga = v;
	return true;
}


bool ConsoleOutput::startThread() {

	if(_videoThread != 0 || _cursorBlink != 0)
		return false;

	_videoMatrix = _vga->getVMem();
	
	pthread_create(&_videoThread, NULL, ConsoleOutput::_mainThread, this);

	pthread_create(&_cursorBlink, NULL, ConsoleOutput::_blinkThread, this);

}

void* ConsoleOutput::_mainThread(void *This_par){

	ConsoleOutput* This = (ConsoleOutput*)This_par;

	cout<<HIDE_CURSOR;
	cout<<CLEAR;


	while(true){

		cout<<CURSOR_START;
		winsize ws;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

        uint16_t curr_rows = ws.ws_row;
        uint16_t curr_cols = ws.ws_col;
        uint16_t minRows = This->_min(ROWS, curr_rows);
        uint16_t minCols = This->_min(COLS, curr_cols);


        uint16_t cursorIndex = This->_vga->cursorPosition();

		uint16_t cursorX = cursorIndex % COLS;
		uint16_t cursorY = floor( cursorIndex / COLS );

        for (int i = 0; i < minRows ; i++) {
			for(int j = 0; j< minCols; j++){

			    uint16_t temp = This->_videoMatrix[i*COLS + j];
			    char c = (char)temp;
			    char textC =(char) ( (temp>>8) & 0x000f);
			    char backg = (char) ((temp & 0x7000)>>12);
		 		char blinkC = (char) ( (temp & 0x8000)>>15);

			    string toPrint = "\033[" + This->_getTextColor((uint32_t)textC) + ';' + This->_getBackgroundColor((uint32_t)backg) + 'm';
			    		 	    
			    if((uint32_t)blinkC){

					pthread_mutex_lock(&(This->_cursorMutex));
					toPrint += (This->_isBlinking)? c : ' ';
					pthread_mutex_unlock(&(This->_cursorMutex));
			    }else
			   		toPrint += c;


			   	if( (cursorY*COLS + cursorX) == (i*COLS+j)){

			   		pthread_mutex_lock(&(This->_cursorMutex));

			   		if(This->_isBlinking)

			   			cout<<UNDERLINED<<toPrint<<RESTORE;

			   		else

			   			cout<<toPrint;

			   		pthread_mutex_unlock(&(This->_cursorMutex));

			   	}else

			    	cout<<toPrint;
		   
 			}

			if(i != minRows -1)

				cout<<STANDARD_BACKGROUND<<endl;
			
			else{

				cout<<STANDARD_BACKGROUND;
				fflush(stdout);
			}
        }
		usleep(REFRESH_TIME);

	}


}


void* ConsoleOutput::_blinkThread(void *param){

	ConsoleOutput* This = (ConsoleOutput*)param;

	while(true){
	
		pthread_mutex_lock(&(This->_cursorMutex));

		This->_isBlinking = !This->_isBlinking;

		pthread_mutex_unlock(&(This->_cursorMutex));

		usleep(BLINK_TIME);
	}

}


ConsoleOutput::colorTable ConsoleOutput::color_t{

		{ //supportedTextColor

			"30", "34", "32", "36", 
			"31", "35", "33", "37", 
			"1;30", "1;34", "1;32", "1;36", 
			"1;31", "1;35", "1;33", "1;37"
			
		},

		{ //supportedBackgroundColor

		//	black  blue green cyan
			"40", "44", "42", "46", 
		//	red magenta brown light-grey	
			"41", "45", "43", "47"
		}

};

string ConsoleOutput::_getTextColor(uint32_t code){

	if(code >= color_t.TEXT_SIZE)
		return color_t.supportedTextColor[0];

	return color_t.supportedTextColor[code];
}


string ConsoleOutput::_getBackgroundColor(uint32_t code){

	if(code >= color_t.BACKGROUND_SIZE)
		return color_t.supportedBackgroundColor[0];
	
	return color_t.supportedBackgroundColor[code];

}



