#include "header/shared_mem.h"
#include "header/shm_manager.h"

#define _SO_LATO ((double)_get_constants(0))
#define _SO_DAYS ((int)_get_constants(1))
#define _SO_NAVI (__SO_NAVI)
#define _SO_PORTI (__SO_PORTI)
#define _SO_MERCI (__SO_MERCI)
#define _SO_STORM_DURATION (__SO_STORM_DURATION)
#define _SO_SWELL_DURATION (__SO_SWELL_DURATION)
#define _SO_MAELSTROM (__SO_MAELSTROM)
#define _SO_FILL (__SO_FILL)
#define _SO_BANCHINE (__SO_BANCHINE)
#define _SO_LOADSPEED (__SO_LOADSPEED)
#define _SO_SIZE (__SO_SIZE)
#define _SO_SPEED (__SO_SPEED)
#define _SO_CAPACITY (__SO_CAPACITY)
#define _SO_MIN_VITA (__SO_MIN_VITA)
#define _SO_MAX_VITA (_SO_MAX_VITA)

#define CHECK_EXIST

struct data *_data;
struct cargo *_data_cargo;

void initialize_if_needed(){
	/* initialize by key (semaphors and shm) */

	/* Attach */
}

double _get_constants(int type_const){
	/*Every get and set start with if (struct = Void * -1) initialize shared */
	/* obtain */
	switch (type_const){
		case 0:
			return _data->
	}
}
