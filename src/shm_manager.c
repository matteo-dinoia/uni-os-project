#include "header/shared_mem.h"
#include "header/shm_manager.h"


#define CHECK_EXIST

struct general *_data;
struct cargo *_data_cargo;

void initialize_if_needed(){
	/* initialize by key (semaphors and shm) */

	/* Attach */
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
