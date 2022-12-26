#define _GNU_SOURCE
#include <stdlib.h>
#include "header/shared_mem.h"
#include "header/shm_manager.h"
#include "header/utils.h"

/* Shared memory keys */
#define KEY_SHM_GENERAL 0x10ff
#define KEY_SHM_PORT 0x20ff
#define KEY_SHM_SHIP 0x30ff
#define KEY_SHM_CARGO 0x40ff
#define KEY_SHM_SHOP 0x50ff
/*  */
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

/* Global variables */
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;
struct cargo *_data_cargo;
struct shop *_data_shop;
id_shared_t _id_data;
id_shared_t _id_port;
id_shared_t _id_ship;
id_shared_t _id_cargo;
id_shared_t _id_shop;


void initialize_shm_manager(int permissions)
{
	/* initialize by key (semaphors and shm) */
	/* TODO: FIX SIZES OF ID */
	_id_data = shmget(KEY_SHM_GENERAL, sizeof(*_data), 0600);
	_id_port = shmget(KEY_SHM_PORT, sizeof(*_data_port), 0600);
	_id_ship = shmget(KEY_SHM_SHIP, sizeof(*_data_ship), 0600);
	_id_cargo = shmget(KEY_SHM_CARGO, sizeof(*_data_cargo), 0600);
	_id_shop = shmget(KEY_SHM_SHOP, sizeof(*_data_shop), 0600);

	/* Attach */
	_data = shmat(_id_data, NULL, 0);
	_data_port = shmat(_id_port, NULL, 0);
	_data_ship = shmat(_id_ship, NULL, 0);
	_data_cargo = shmat(_id_cargo, NULL, 0);
	_data_shop = shmat(_id_shop, NULL, 0);

}

double _get_constants(int type_const){
	/*Every get and set start with if (struct = Void * -1) initialize shared */
	/* obtain */
	switch (type_const){
		case 0:
			return _data->
	}
}
