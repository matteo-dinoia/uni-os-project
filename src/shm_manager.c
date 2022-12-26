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

double get_constants(int type_const){
	/*Every get and set start with if (struct = Void * -1) initialize shared */
	/* Check pemissions only in write */
	/* obtain */
	switch (type_const % 16){
		case 0: return _data->so_lato;
		case 1: return _data->so_days;
		case 2: return _data->so_navi;
		case 3: return _data->so_porti;
		case 4: return _data->so_merci;
		case 5: return _data->so_storm_duration;
		case 6: return _data->so_swell_duration;
		case 7: return _data->so_maelstrom;
		case 8: return _data->so_fill;
		case 9: return _data->so_banchine;
		case 10: return _data->so_loadspeed;
		case 11: return _data->so_size;
		case 12: return _data->so_speed;
		case 13: return _data->so_capacity;
		case 14: return _data->so_min_vita;
		case 15: return _data->so_max_vita;
	}
}

/* COORD */
struct coord get_coord_port(int id){
	/* COntrols */
	return _data_port->coordinates;
}
void set_coord_port(int id, double x, double y){
	_data_port->coordinates.x = x;
	_data_port->coordinates.x = y;
}
struct coord get_coord_ship(int id){
	return _data_ship->coordinates;
}
void set_coord_ship(int id, double x, double y){
	_data_ship->coordinates.x = x;
	_data_ship->coordinates.x = y;
}

/* DAY */
int getday(){
	return _data->today;
}

void setday(int day){
	_data->today = day;
}
