#include "keyboard.h"
#include <iostream>

using namespace std;

keyboard::keyboard() : RBR(0), TBR(0), STR(0), CMR(0), enabled(false), interrupt_enabled(false),
	buffer_head_pointer(0), buffer_tail_pointer(0), buffer_element_count(0), interrupt_raised(false) {
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
			// chiediamo a RBR e FI di aggiornare il proprio stato:
			// RBR è aggiornato con il valore da leggere,
			next_RBR();
			result = RBR;
			// FI è aggiornato in base a se ci sono ancora keycode
			// da prelevare dal buffer
			update_FI();
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
			if(TBR & 0x10)	// bit 4: disabilitazione tastiera
				enabled = false;
			else
				enabled = true;

			if(TBR & 0x01)	// bit 0: abilitazione interruzioni
				interrupt_enabled = true;
			else
				interrupt_enabled = false;
		break;
	}
}

void keyboard::next_RBR()
{
	// se abbiamo caratteri nel buffer alziamo FI
	// e aggiorniamo il contenuto di RBR prelevando
	// il primo keycode dalla coda
	if(buffer_element_count != 0)
	{
		RBR = internal_buffer[buffer_tail_pointer];
		buffer_tail_pointer = (buffer_tail_pointer + 1) % INTERNAL_BUFFER_SIZE;
		buffer_element_count--;
	}
}

void keyboard::update_FI()
{
	// se abbiamo caratteri nel buffer alziamo FI
	if(buffer_element_count != 0)
		STR |= FI_MASK;
	// altrimenti abbassiamolo
	else
		STR &= ~FI_MASK;
}

void keyboard::insert_keycode_event(uint8_t keycode)
{
	pthread_mutex_lock(&mutex);

	#ifdef DEBUG_LOG
	logg << "keyboard: ricevuto " << (unsigned int)keycode << endl;
	#endif

	if(!enabled)
		goto err;

	// condizione di coda piena
	if(buffer_element_count == INTERNAL_BUFFER_SIZE)
	{
		logg << "keyboard: droppato per coda piena" << endl;
		goto err;
	}

	// inseriamo il keycode nel buffer (coda)
	#ifdef DEBUG_LOG
	logg << "keyboard: inserito in coda" << endl;
	#endif

	internal_buffer[buffer_head_pointer] = keycode;
	buffer_head_pointer = (buffer_head_pointer + 1) % INTERNAL_BUFFER_SIZE;
	buffer_element_count++;

	// alziamo il bit FI, ma lo deleghiamo a update_FI() per gestire eventuali
	// eventi di interruzione
	update_FI();

err:
	pthread_mutex_unlock(&mutex);
}