#include "keyboard.h"
#include <iostream>

using namespace std;

keyboard::keyboard() : RBR(0), TBR(0), STR(0), CMR(0), enabled(false), interrupt_enabled(false), interrupt_raised(false) {
	pthread_mutex_init(&mutex, NULL);
}

void keyboard::write_reg_byte(io_addr addr, uint8_t val)
{
	pthread_mutex_lock(&mutex);

	switch(addr) {
		case CMR_addr: CMR=val; break;
		case TBR_addr: TBR=val; process_cmd(); break;
	}

	pthread_mutex_unlock(&mutex);
}

uint8_t keyboard::read_reg_byte(io_addr addr)
{
	pthread_mutex_lock(&mutex);

	uint8_t result = 0;

	switch(addr) {
		case RBR_addr:
			STR &= ~FI_MASK;
			interrupt_raised = false;
			result = RBR;
			break;
		case STR_addr:
			result= STR;
	}

	pthread_mutex_unlock(&mutex);

	return result;
}

void keyboard::process_cmd()
{
	switch(CMR) {
		// === configurazione tastiera ===
		case 0x60:
			if(TBR & 0x10)	// bit 4: abilitazione tastiera
				enabled = true;
			else
				enabled = false;

			if(TBR & 0x01)	// bit 0: abilitazione interruzioni
				interrupt_enabled = true;
			else
				interrupt_enabled = false;
		break;
	}
}

void keyboard::insert_keycode_event(uint8_t keycode)
{
	pthread_mutex_lock(&mutex);

	cout << "keyboard: ricevuto " << (unsigned int)keycode << endl;

	if(!enabled)
		goto err;

	// mettiamo il keycode nel registro RBR e segnaliamo che il buffer è pieno
	RBR = keycode;
	STR |= FI_MASK;

	// provochiamo un'interruzione
	if(interrupt_enabled)
		interrupt_raised = true;

err:
	pthread_mutex_unlock(&mutex);
}