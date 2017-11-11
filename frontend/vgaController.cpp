#include "vgaController.h"

using namespace std;

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
					cerr<<"Registro selezionato non valido"<<endl;
			}

		break;

		default:

			cerr<<"Indirizzo selezionato non valido"<<endl;

	}

	pthread_mutex_unlock(&mutex);

}
