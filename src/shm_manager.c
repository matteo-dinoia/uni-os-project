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

	/* Initialize shm data if needed */
	if (base_data != NULL){
		bzero(_data_port, sizeof(*_data_port) * SO_PORTI);
		bzero(_data_ship, sizeof(*_data_ship) * SO_NAVI);
		bzero(_data_cargo , sizeof(*_data_cargo) * SO_MERCI);
		bzero(_data_shop, sizeof(*_data_shop) * SO_MERCI * SO_PORTI);
		_initialize_data();
	}
}

void _initialize_data(){
	int i, to_add, daily, n_docks;
	struct port *current_port;
	struct ship *current_ship;
	struct cargo *current_cargo;

	/* Initialize ports data */
	daily = SO_FILL / (SO_DAYS * SO_PORTI);
	for (i = 0; i < SO_PORTI; i++){
		current_port = &_data_port[i];

		to_add = (i < (SO_FILL % (SO_DAYS * SO_PORTI))) ? 1 : 0;
		current_port->daily_restock_capacity = daily + to_add;

		if (i<4){
			/* ports in 4 corner */
			current_port->x = i % 2 != 0 ? SO_LATO : 0;
			current_port->y = i < 2 ? SO_LATO : 0;
		}
		else{
			current_port->x = RANDOM_DOUBLE(0, SO_LATO);
			current_port->y = RANDOM_DOUBLE(0, SO_LATO);
		}

		n_docks = RANDOM(1, SO_BANCHINE);
		current_port->dump_dock_tot = n_docks;
		semctl(_data->id_sem_docks, i, SETVAL, n_docks);

		current_port->dump_had_swell = FALSE;
	}

	/* Initialize ships data */
	for (i = 0; i < SO_NAVI; i++){
		current_ship = &_data_ship[i];

		current_ship->x = RANDOM_DOUBLE(0, SO_LATO);
		current_ship->y = RANDOM_DOUBLE(0, SO_LATO);

		/* Initializing ship dump */
		current_ship->capacity = SO_CAPACITY;
	}

	/* Initialize cargo data */
	for (i = 0; i < SO_MERCI; i++){
		current_cargo = &_data_cargo[i];

		current_cargo->weight_batch = RANDOM(1, SO_SIZE);
		current_cargo->shelf_life = RANDOM(SO_MIN_VITA, SO_MAX_VITA);

		/* Semaphore */
		semctl(_data->id_sem_cargo, i, SETVAL, 1);
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

void print_dump_data()
{
	int port, type, ship, cargo_type, cargo_in_port, quantity;
	int tot_port_swell = 0;
	int tot_ship_storm = 0, tot_ship_maelstrom = 0, tot_ship_dock = 0, tot_ship_empty = 0, tot_ship_cargo = 0;
	int tot_cargo_port = 0, tot_cargo_ship = 0, tot_cargo_del = 0, tot_cargo_exp_ship = 0, tot_cargo_exp_port = 0;

	dprintf(1, "\n\n================================[DAY %3d]=================================\n", _data->today);

	/* Dumps things */
	dprintf(1, "\n================================[DUMPS]===================================\n");

	dprintf(1, "[PORTS]\n");
	for (port = 0; port < SO_PORTI; port++){
		cargo_in_port = 0;
		dprintf(1, "|----(Port: %d) tot_docks: %d, used_docks: %d, swell: %d. Cargo:\n", port, _data_port[port].dump_dock_tot,
				_data_port[port].dump_dock_tot - semctl(_data->id_sem_docks, port, GETVAL), _data_port[port].dump_had_swell);
		tot_port_swell += _data_port[port].dump_had_swell;

		for (type = 0; type < SO_MERCI; type++){
			quantity = _data_supply_demand[SO_MERCI * port + type].quantity;
			cargo_in_port = quantity > 0 ? quantity : 0;
			dprintf(1, "|    |----(Cargo type: %d) in_port: %d, sent: %d, received: %d\n", type, cargo_in_port,
					_data_supply_demand[SO_MERCI * port + type].dump_tot_sent,
					_data_supply_demand[SO_MERCI * port + type].dump_tot_received);
		}
		dprintf(1, "|\n");
	}
	dprintf(1, "|--PORTS TOTALS: tot_swell: %d\n\n", tot_port_swell);

	dprintf(1, "[SHIPS]\n");
	for (ship = 0; ship < SO_NAVI; ship++){
		dprintf(1, "|----(Ship: %d) is_at_dock: %d, had_storm: %d, had_maeltrom: %d\n", ship, _data_ship[ship].dump_is_at_dock,
				_data_ship[ship].dump_had_storm, _data_ship[ship].dump_had_maelstrom);
		if (_data_ship[ship].dump_is_at_dock) tot_ship_dock++;
		else if (_data_ship[ship].capacity == SO_CAPACITY) tot_ship_empty++;
		else tot_ship_cargo++;
		tot_ship_storm += _data_ship[ship].dump_had_storm;
		tot_ship_maelstrom += _data_ship[ship].dump_had_maelstrom;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--SHIPS TOTALS: tot_at_dock: %d, tot_cargo: %d, tot_empty: %d, tot_storm: %d, tot_maeltrom: %d\n\n",
			tot_ship_dock, tot_ship_cargo, tot_ship_empty, tot_ship_storm, tot_ship_maelstrom);

	dprintf(1, "[CARGO]\n");
	for (cargo_type = 0; cargo_type < SO_MERCI; cargo_type++){
		dprintf(1, "|----(Cargo type: %d) tot_in_ports: %d, tot_in_ships: %d, tot_delivered: %d, tot_expired_port: %d, tot_expired_ship: %d\n",
				cargo_type, _data_cargo[cargo_type].dump_at_port, _data_cargo[cargo_type].dump_in_ship, _data_cargo[cargo_type].dump_tot_delivered,
				_data_cargo[cargo_type].dump_exipered_port, _data_cargo[cargo_type].dump_exipered_ship);

				/* Totals */
				tot_cargo_port += _data_cargo[cargo_type].dump_at_port;
				tot_cargo_ship += _data_cargo[cargo_type].dump_in_ship;
				tot_cargo_del += _data_cargo[cargo_type].dump_tot_delivered;
				tot_cargo_exp_port += _data_cargo[cargo_type].dump_exipered_port;
				tot_cargo_exp_ship += _data_cargo[cargo_type].dump_exipered_ship;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--CARGO TOTALS: tot_cargo_port: %d, tot_cargo_ship: %d, tot_cargo_delivered: %d, tot_cargo_expired_port: %d, tot_cargo_expired_ship: %d\n\n",
			tot_cargo_port, tot_cargo_ship, tot_cargo_del, tot_cargo_exp_port, tot_cargo_exp_ship);

	/* Shop things */
	dprintf(1, "================================[SHOP]====================================\n");
	for (port = 0; port < SO_PORTI; port++){
		dprintf(1, "PORT %d:   ", port);
		for (type = 0; type < SO_MERCI; type++){
			dprintf(1, "(t: %d)%d ", type,
					_data_supply_demand[SO_MERCI * port + type].quantity);
		}
		dprintf(1, "\n");
	}
	dprintf(1, "\n================================[END SHOP]================================\n\n\n");

	if(_data->today >= SO_DAYS)
		close_all("================================[END SIMULATION]================================\n", EXIT_SUCCESS);
}

/* GETTER */
/* Day */
int getday(){return _data->today;}
/* Port */
struct coord get_coord_port(int port_id){return _data_port[port_id].coordinates;}
int get_port_daily_restock(int port_id){return _data_port[port_id].daily_restock_capacity;}
int get_port_tot_dock(int port_id){return _data_port[port_id].dump_dock_tot;}
bool_t had_port_swell(int port_id){return _data_port[port_id].dump_had_swell;}
/* Ship */
bool_t is_ship_dead(int ship_id){return _data_ship[ship_id].is_dead;}
struct coord get_coord_ship(int ship_id){return _data_ship[ship_id].coordinates;}
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
int get_shop_quantity(int port_id, int cargo_id){return _data_shop[SO_MERCI * port_id+ cargo_id].quantity;}
int get_shop_tot_sent(int port_id, int cargo_id){return _data_shop[SO_MERCI * port_id+ cargo_id].dump_tot_sent;}
int get_shop_tot_received(int port_id, int cargo_id){return _data_shop[SO_MERCI * port_id+ cargo_id].dump_tot_received;}

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
void increase_day(){_data->today++;}