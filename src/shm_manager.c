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

double get_constants(int type_const)
{
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
struct coord get_coord_port(int id)
{
	/* COntrols */
	return _data_port->coordinates;
}
void set_coord_port(int id, double x, double y)
{
	_data_port->coordinates.x = x;
	_data_port->coordinates.x = y;
}
struct coord get_coord_ship(int id)
{
	return _data_ship->coordinates;
}
void set_coord_ship(int id, double x, double y){
	_data_ship->coordinates.x = x;
	_data_ship->coordinates.x = y;
}

/* DAY */
int getday()
{
	return _data->today;
}

void setday(int day){
	_data->today = day;
}

/* PORT */
int get_port(int port_id, int var_type)
{
	switch(var_type){
		case VAR_PID: return _data_port[port_id].pid;
		case VAR_DAILY_RESTOCK: return _data_port[port_id].daily_restock_capacity;
		case VAR_DUMP_DOCK_TOT: return _data_port[port_id].dump_dock_tot;
		case VAR_DUMP_HAD_SWELL: return _data_port[port_id].dump_had_swell;
		default: return -1;
	}
}

void set_port(int port_id, int var_type, int var)
{
	switch(var_type){
		case VAR_PID:
			_data_port[port_id].pid = var;
			break;
		case VAR_DAILY_RESTOCK:
			_data_port[port_id].daily_restock_capacity = var;
			break;
		case VAR_DUMP_DOCK_TOT:
			_data_port[port_id].dump_dock_tot = var;
			break;
		case VAR_DUMP_HAD_SWELL:
			_data_port[port_id].dump_had_swell = var;
			break;
	}
}

/* SHIP */
int get_ship(int ship_id, int var_type)
{
	switch(var_type){
		case VAR_PID: return _data_ship[ship_id].pid;
		case VAR_IS_MOVING: return _data_ship[ship_id].is_moving;
		case VAR_CAPACITY: return _data_ship[ship_id].capacity;
		case VAR_DUMP_IS_AT_DOCK: return _data_ship[ship_id].dump_is_at_dock;
		case VAR_DUMP_HAD_STORM: return _data_ship[ship_id].dump_had_storm;
		case VAR_DUMP_HAD_MAELSTROM: return _data_ship[ship_id].dump_had_maelstrom;
		default: return -1;
	}
}

void set_ship(int ship_id, int var_type, int var)
{
	switch(var_type){
		case VAR_PID:
			_data_ship[ship_id].pid = var;
			break;
		case VAR_IS_MOVING:
			_data_ship[ship_id].is_moving = var;
			break;
		case VAR_CAPACITY:
			_data_ship[ship_id].capacity = var;
			break;
		case VAR_DUMP_IS_AT_DOCK:
			_data_ship[ship_id].dump_is_at_dock = var;
			break;
		case VAR_DUMP_HAD_STORM:
			_data_ship[ship_id].dump_had_storm = var;
			break;
		case VAR_DUMP_HAD_MAELSTROM:
			_data_ship[ship_id].dump_had_maelstrom = var;
			break;
	}
}

/* CARGO */
int get_cargo(int cargo_id, int var_type)
{
	switch(var_type){
		case VAR_WEIGHT_BATCH: return _data_cargo[cargo_id].weight_batch;
		case VAR_SHELF_LIFE: return _data_cargo[cargo_id].shelf_life;
		case VAR_DUMP_AT_PORT: return _data_cargo[cargo_id].dump_at_port;
		case VAR_DUMP_IN_SHIP: return _data_cargo[cargo_id].dump_in_ship;
		case VAR_DUMP_EXPIRED_PORT: return _data_cargo[cargo_id].dump_exipered_port;
		case VAR_DUMP_EXPIRED_SHIP: return _data_cargo[cargo_id].dump_exipered_ship;
		case VAR_DUMP_TOT_DELIVERED: return _data_cargo[cargo_id].dump_tot_delivered;
		default: return -1;
	}
}

void set_cargo(int cargo_id, int var_type, int var)
{
	switch(var_type){
		case VAR_WEIGHT_BATCH:
			_data_cargo[cargo_id].weight_batch = var;
			break;
		case VAR_SHELF_LIFE:
			_data_cargo[cargo_id].shelf_life = var;
			break;
		case VAR_DUMP_AT_PORT:
			_data_cargo[cargo_id].dump_at_port = var;
			break;
		case VAR_DUMP_IN_SHIP:
			_data_cargo[cargo_id].dump_in_ship = var;
			break;
		case VAR_DUMP_EXPIRED_PORT:
			_data_cargo[cargo_id].dump_exipered_port = var;
			break;
		case VAR_DUMP_EXPIRED_SHIP:
			_data_cargo[cargo_id].dump_exipered_ship = var;
			break;
		case VAR_DUMP_TOT_DELIVERED:
			_data_cargo[cargo_id].dump_tot_delivered = var;
			break;
	}
}

/* SHOP */
int get_shop(int port_id, int cargo_id, int var_type)
{
	switch(var_type){
		case VAR_QUANTITY: return _data_shop[port_id].quantity;
		case VAR_DUMP_TOT_SENT: return _data_shop[port_id].dump_tot_sent;
		case VAR_DUMP_TOT_RECEIVED: return _data_shop[port_id].dump_tot_received;
		default: return -1;
	}
}

void set_shop(int port_id, int cargo_id, int var_type, int var)
{
	switch(var_type){
		case VAR_QUANTITY:
			_data_shop[port_id].quantity = var;
			break;
		case VAR_DUMP_TOT_SENT:
			_data_shop[port_id].dump_tot_sent = var;
			break;
		case VAR_DUMP_TOT_RECEIVED:
			_data_shop[port_id].dump_tot_received = var;
			break;
	}
}
