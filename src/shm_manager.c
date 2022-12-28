#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "header/shared_mem.h"
#include "header/utils.h"
#include "header/shm_manager.h"

/* Macros */
/* Default value*/
#define NULL_SHM ((void *)-1)
#define NULL_ID -1
/* Shared memory keys */
#define KEY_SHM_GENERAL 0x10ff
#define KEY_SHM_PORT 0x20ff
#define KEY_SHM_SHIP 0x30ff
#define KEY_SHM_CARGO 0x40ff
#define KEY_SHM_SHOP 0x50ff

/* Global variables */
struct general *_data = NULL_SHM;
struct port *_data_port = NULL_SHM;
struct ship *_data_ship = NULL_SHM;
struct cargo *_data_cargo = NULL_SHM;
struct shop *_data_shop = NULL_SHM;
id_shared_t _id_data = NULL_ID;
id_shared_t _id_port = NULL_ID;
id_shared_t _id_ship = NULL_ID;
id_shared_t _id_cargo = NULL_ID;
id_shared_t _id_shop = NULL_ID;

/* Must be initialized by master before anyone accessing it */
void initialize_shm_manager(int permissions, const struct general *base_data)
{
	/* Initialize and attach main */
	_id_data = shmget(KEY_SHM_GENERAL, sizeof(*_data), 0600);
	_data = shmat(_id_data, NULL, 0);
	if (base_data != NULL){
		memcpy(_data, base_data, sizeof(*base_data));
	}

	/* Initialize by key */
	_id_port = shmget(KEY_SHM_PORT, sizeof(*_data_port) * SO_PORTI, 0600);
	_id_ship = shmget(KEY_SHM_SHIP, sizeof(*_data_ship) * SO_NAVI, 0600);
	_id_cargo = shmget(KEY_SHM_CARGO, sizeof(*_data_cargo) * SO_MERCI, 0600);
	_id_shop = shmget(KEY_SHM_SHOP, sizeof(*_data_shop) * SO_MERCI * SO_PORTI, 0600);

	/* Attach */
	_data_port = shmat(_id_port, NULL, 0);
	_data_ship = shmat(_id_ship, NULL, 0);
	_data_cargo = shmat(_id_cargo, NULL, 0);
	_data_shop = shmat(_id_shop, NULL, 0);

	if (base_data != NULL){
		bzero(_data_port, sizeof(*_data_port) * SO_PORTI);
		bzero(_data_ship, sizeof(*_data_ship) * SO_NAVI);
		bzero(_data_cargo , sizeof(*_data_cargo) * SO_MERCI);
		bzero(_data_shop, sizeof(*_data_shop) * SO_MERCI * SO_PORTI);
	}

}

void close_shm_manager(){
	/* Detach */
	shmdt(_data);
	shmdt(_data_port);
	shmdt(_data_ship);
	shmdt(_data_cargo);
	shmdt(_data_shop);

	/* Mark for removal shared memory */
	shmctl(_id_data, IPC_RMID, NULL);
	shmctl(_id_port, IPC_RMID, NULL);
	shmctl(_id_ship, IPC_RMID, NULL);
	shmctl(_id_cargo, IPC_RMID, NULL);
	shmctl(_id_shop, IPC_RMID, NULL);
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

/* GETTER */
/* Day */
int getday(){return _data->today;}
/* Port */
struct coord get_coord_port(int id){return _data_port->coordinates;}
int get_port_daily_restock(int port_id){return _data_port[port_id].daily_restock_capacity;}
int get_port_tot_dock(int port_id){return _data_port[port_id].dump_dock_tot;}
bool_t had_port_swell(int port_id){return _data_port[port_id].dump_had_swell;}
/* Ship */
struct coord get_coord_ship(int id){return _data_ship->coordinates;}
bool_t is_ship_moving(int ship_id){return _data_ship[ship_id].is_moving;}
int get_ship_capacity(int ship_id){return _data_ship[ship_id].capacity;}
bool_t is_ship_at_port(int ship_id){return _data_ship[ship_id].dump_is_at_dock;}
bool_t had_ship_storm(int ship_id){return _data_ship[ship_id].dump_had_storm;}
bool_t had_ship_maelstrom(int ship_id){return _data_ship[ship_id].dump_had_maelstrom;}
/* Cargo */
int get_cargo_weight_batch(int cargo_id){return _data_cargo[cargo_id].weight_batch;}
int get_cargo_shelf_life(int cargo_id){return _data_cargo[cargo_id].shelf_life;}
int get_cargo_at_port(int cargo_id){return _data_cargo[cargo_id].dump_at_port;}
int get_cargo_in_ship(int cargo_id){return _data_cargo[cargo_id].dump_in_ship;}
int get_cargo_expired_port(int cargo_id){return _data_cargo[cargo_id].dump_exipered_port;}
int get_cargo_expired_ship(int cargo_id){return _data_cargo[cargo_id].dump_exipered_ship;}
int get_cargo_tot_delivered(int cargo_id){return _data_cargo[cargo_id].dump_tot_delivered;}
/* Shop */
int get_shop_quantity(int port_id, int cargo_id){return _data_shop[port_id].quantity;}
int get_shop_tot_sent(int port_id, int cargo_id){return _data_shop[port_id].dump_tot_sent;}
int get_shop_tot_received(int port_id, int cargo_id){return _data_shop[port_id].dump_tot_received;}

/* "SETTER" */
void set_coord_port(int id, double x, double y)
{
	_data_port->coordinates.x = x;
	_data_port->coordinates.x = y;
}
void set_coord_ship(int id, double x, double y){
	_data_ship->coordinates.x = x;
	_data_ship->coordinates.x = y;
}
void setday(int day){_data->today = day;}