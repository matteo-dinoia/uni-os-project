#include "header/shared_mem.h"
#include "header/shm_manager.h"

#define _SO_DAYS _get_constants(DAYS)

struct cargo *_data_cargo;

void _initialize_shared(){
	/* initialize by key (semaphors and shm) */

	/* Attach */
}

int _get_constants(int value){
	/*Every get and set start with if (struct = Void * -1) initialize shared */
	/* obtain */
}

