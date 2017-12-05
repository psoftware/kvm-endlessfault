#include "vgaController.h"


VGAController::VGAController() : IND(0), DAT(0), CUR_HIGH(0), CUR_LOW(0){

	pthread_mutex_init(&mutex, NULL);

}


void VGAController:: write_reg_byte(io_addr addr, uint8_t val){
	
	pthread_mutex_lock(&mutex);

	switch(addr) {

		case IND_addr:

			IND = val;

		break;

		case DAT_addr:

			DAT = val;

			switch(IND){
				case CUR_HIGH_ind:
					CUR_HIGH = DAT;
				break;

				case CUR_LOW_ind:
					CUR_LOW = DAT;	
				break;

				default:
					#ifdef DEBUG_LOG
					logg<<"VGA : Register not valid"<<endl;
					#endif
			}

		break;

		default:
			#ifdef DEBUG_LOG
			logg<<"VGA : Address not valid"<<endl;
			#endif

	}

	pthread_mutex_unlock(&mutex);

}


uint8_t VGAController::read_reg_byte(io_addr addr)
{
	pthread_mutex_lock(&mutex);

	uint8_t result = 0;

	switch(addr) {
		
		case IND_addr:
			result = IND;
		break;

		case DAT_addr:

			switch(IND){
				case CUR_HIGH_ind:
					DAT = CUR_HIGH;
					result = DAT;
				break;

				case CUR_LOW_ind:
					DAT = CUR_LOW;
					result = DAT;	
				break;

				default:
					#ifdef DEBUG_LOG
					logg<<"VGA : Register not valid"<<endl;
					#endif
			}		

		break;

		default:
			#ifdef DEBUG_LOG
			logg<<"VGA : Address not valid"<<endl;
			#endif
	}

	pthread_mutex_unlock(&mutex);

	return result;
}




uint16_t VGAController::cursorPosition(){

	pthread_mutex_lock(&mutex);

	uint16_t index = (((uint16_t)CUR_HIGH) << 8 ) + ( (uint16_t) CUR_LOW );

	pthread_mutex_unlock(&mutex);

	return index;
}


void VGAController::setVMem(uint16_t* mem){

	_memoryStart = mem;

}


uint16_t* VGAController::getVMem(){


	return _memoryStart;
}