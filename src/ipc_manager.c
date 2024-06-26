#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "header/shared_mem.h"
#include "header/semaphore.h"
#include "header/message.h"
#include "header/utils.h"
#include "header/ipc_manager.h"

/* Macros */
#define DATA_SHOP(port_id, cargo_type) (_data_shop[(port_id) * SO_MERCI + (cargo_type)])
/* Default value*/
#define NULL_SHM ((void *)-1)
#define NULL_ID -1

/* Global variables */
bool_t is_initialized = FALSE;
/* SHM */
struct general *_data = NULL_SHM;
struct port *_data_port = NULL_SHM;
struct ship *_data_ship = NULL_SHM;
struct cargo *_data_cargo = NULL_SHM;
struct shop *_data_shop = NULL_SHM;
/* Ids */
id_shared_t _id_data = NULL_ID;
id_shared_t _id_port = NULL_ID;
id_shared_t _id_ship = NULL_ID;
id_shared_t _id_cargo = NULL_ID;
id_shared_t _id_shop = NULL_ID;
/* MSG port in and out */
id_shared_t _id_msg_in_ports = NULL_ID;
id_shared_t _id_msg_out_ports = NULL_ID;
/* SEM: Start and docks */
id_shared_t _id_sem = NULL_ID;
id_shared_t _id_sem_docks = NULL_ID;
id_shared_t _id_sem_cargo = NULL_ID;

/* Prototype private */
void _initialize_data();

/* Must be initialized by master before anyone accessing it */
void initialize_ipc_manager(const struct general *base_data) /* Put null if not initalizing */
{
	/* Wait master if needed */
	if(base_data == NULL){
		_id_sem = semget(KEY_SEM_START, 1, 0600 | IPC_CREAT);
		execute_single_sem_oper(_id_sem, 0, 0);
	}

	/* Initialize and attach data */
	_id_data = shmget(KEY_SHM_GENERAL, sizeof(*_data), 0600 | IPC_CREAT);
	_data = shmat(_id_data, NULL, 0);
	if (base_data != NULL)
		memcpy(_data, base_data, sizeof(*base_data));


	/* Initialize by key */
	_id_port = shmget(KEY_SHM_PORT, sizeof(*_data_port) * SO_PORTI, 0600 | IPC_CREAT);
	_id_ship = shmget(KEY_SHM_SHIP, sizeof(*_data_ship) * SO_NAVI, 0600 | IPC_CREAT);
	_id_cargo = shmget(KEY_SHM_CARGO, sizeof(*_data_cargo) * SO_MERCI, 0600 | IPC_CREAT);
	_id_shop = shmget(KEY_SHM_SHOP, sizeof(*_data_shop) * SO_MERCI * SO_PORTI, 0600 | IPC_CREAT);

	/* MSG port in and out */
	_id_msg_in_ports = msgget(KEY_MSG_IN_PORT, 0600 | IPC_CREAT);
	_id_msg_out_ports = msgget(KEY_MSG_OUT_PORT, 0600 | IPC_CREAT);
	/* SEM: Start and docks */
	_id_sem = semget(KEY_SEM_START, 1, 0600 | IPC_CREAT);
	_id_sem_docks = semget(KEY_SEM_DOCKS, SO_PORTI, 0600 | IPC_CREAT);
	_id_sem_cargo = semget(KEY_SEM_CARGO, SO_MERCI, 0600 | IPC_CREAT);

	/* Attach */
	_data_port = shmat(_id_port, NULL, 0);
	_data_ship = shmat(_id_ship, NULL, 0);
	_data_cargo = shmat(_id_cargo, NULL, 0);
	_data_shop = shmat(_id_shop, NULL, 0);

	/* Initialize shm data if needed */
	if (base_data != NULL)
		_initialize_data();

	is_initialized = TRUE;
}

void _initialize_data()
{
	int i, to_add, daily_tot, n_docks, daily_port;
	double x, y;
	struct port *current_port;
	struct ship *current_ship;
	struct cargo *current_cargo;

	/* Putting to zero everything */
	bzero(_data_port, sizeof(*_data_port) * SO_PORTI);
	bzero(_data_ship, sizeof(*_data_ship) * SO_NAVI);
	bzero(_data_cargo , sizeof(*_data_cargo) * SO_MERCI);
	bzero(_data_shop, sizeof(*_data_shop) * SO_MERCI * SO_PORTI);

	/* Initialize ports data */
	daily_tot = SO_FILL / SO_DAYS;
	if (daily_tot <= 0) daily_tot = 1;
	for (i = 0; i < SO_PORTI; i++){
		current_port = &_data_port[i];

		if (SO_MERCI == 1 && i % 2 == 0){ /* Port even request if only one type */
			daily_port = daily_tot / ((SO_PORTI + 1) / 2);
			to_add = ((int)(i / 2) < daily_tot % ((SO_PORTI + 1) / 2)) ? 1 : 0;
			current_port->daily_restock_demand = daily_port + to_add;
			current_port->daily_restock_supply = 0;

		}else if (SO_MERCI == 1 && i % 2 != 0){ /* Port odd sell if only one type */
			daily_port = daily_tot / (SO_PORTI / 2);
			to_add = ((int)(i / 2) < daily_tot % (SO_PORTI / 2)) ? 1 : 0;
			current_port->daily_restock_supply = daily_port + to_add;
			current_port->daily_restock_demand = 0;
		}else {
			daily_port = daily_tot / SO_PORTI;
			to_add = (i < daily_tot % SO_PORTI) ? 1 : 0;
			current_port->daily_restock_supply = daily_port + to_add;
			current_port->daily_restock_demand = daily_port + to_add;
		}

		if (i<4){
			/* ports in 4 corner */
			x = i % 2 != 0 ? SO_LATO : 0;
			y = i < 2 ? SO_LATO : 0;
		}
		else{
			x = RANDOM_DOUBLE(0, SO_LATO);
			y = RANDOM_DOUBLE(0, SO_LATO);
		}
		current_port->coordinates.x = x;
		current_port->coordinates.y = y;

		n_docks = RANDOM_INCLUDED(1, SO_BANCHINE);
		current_port->dump_dock_tot = n_docks;
		semctl(_id_sem_docks, i, SETVAL, n_docks);
	}

	/* Initialize ships data */
	for (i = 0; i < SO_NAVI; i++){
		current_ship = &_data_ship[i];

		x = RANDOM_DOUBLE(0, SO_LATO);
		y = RANDOM_DOUBLE(0, SO_LATO);
		set_ship_coord(i, x, y);

		/* Initializing ship dump */
		current_ship->capacity = SO_CAPACITY;
	}

	/* Initialize cargo data */
	for (i = 0; i < SO_MERCI; i++){
		current_cargo = &_data_cargo[i];

		current_cargo->weight_batch = RANDOM_INCLUDED(1, SO_SIZE);
		current_cargo->shelf_life = RANDOM_INCLUDED(SO_MIN_VITA, SO_MAX_VITA);

		/* Semaphore */
		semctl(_id_sem_cargo, i, SETVAL, 1);
	}

	/* Other */
	semctl(_id_sem, 0, SETVAL, 1);
	_data->today = 1;
}

void close_ipc_manager()
{
	/* Detach */
	shmdt(_data);
	shmdt(_data_port);
	shmdt(_data_ship);
	shmdt(_data_cargo);
	shmdt(_data_shop);
}

void close_ipc()
{
	/* Closing semaphors */
	semctl(_id_sem, 0, IPC_RMID);
	semctl(_id_sem_docks, 0, IPC_RMID);
	semctl(_id_sem_cargo, 0, IPC_RMID);

	/* Closing message queues */
	msgctl(_id_msg_in_ports, IPC_RMID, NULL);
	msgctl(_id_msg_out_ports, IPC_RMID, NULL);

	/* Mark for removal shared memory */
	shmctl(_id_data, IPC_RMID, NULL);
	shmctl(_id_port, IPC_RMID, NULL);
	shmctl(_id_ship, IPC_RMID, NULL);
	shmctl(_id_cargo, IPC_RMID, NULL);
	shmctl(_id_shop, IPC_RMID, NULL);
}

double get_constants(int type_const)
{
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
	int tot_port_swell = 0, tot_port_docked_ship = 0;
	int tot_ship_storm = 0, tot_ship_maelstrom = 0, tot_ship_dock = 0, tot_ship_empty = 0, tot_ship_cargo = 0, tot_ship_dead = 0, tot_ship_loaded = 0;
	int tot_cargo_port = 0, tot_cargo_ship = 0, tot_cargo_del = 0, tot_cargo_exp_ship = 0, tot_cargo_exp_port = 0, tot_cargo_del_unwanted = 0;

	dprintf(1, "\n\n================================[DAY %3d]=================================\n", get_day());

	/* Dumps things */
	dprintf(1, "\n================================[DUMPS]===================================\n");

	dprintf(1, "[PORTS]\n");
	for (port = 0; port < SO_PORTI; port++){
		cargo_in_port = 0;
		dprintf(1, "|----(Port: %d) used_docks: %d/%d, swell: %d, tot_cargo_sent: %d, tot_cargo_received: %d, ship_docked_until_now: %d, daily_restock: %d (d) %d(s)\n",
				port, _data_port[port].dump_dock_tot - semctl(_id_sem_docks, port, GETVAL), _data_port[port].dump_dock_tot, _data_port[port].dump_had_swell,
				_data_port[port].dump_tot_tons_sent, _data_port[port].dump_tot_tons_received, _data_port[port].dump_ship_arrived, _data_port[port].daily_restock_demand, _data_port[port].daily_restock_supply);
		tot_port_swell += _data_port[port].dump_had_swell;
		tot_port_docked_ship += _data_port[port].dump_ship_arrived;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--PORTS TOTALS: tot_swell: %d tot_ship_docked_until_now: %d\n\n", tot_port_swell, tot_port_docked_ship);

	dprintf(1, "[SHIPS]\n");
	for (ship = 0; ship < SO_NAVI; ship++){
		if (_data_ship[ship].is_dead) tot_ship_dead++;
		else if (_data_ship[ship].dump_is_at_dock) tot_ship_dock++;
		else if (_data_ship[ship].capacity >= SO_CAPACITY) tot_ship_empty++;
		else tot_ship_cargo++;
		tot_ship_storm += _data_ship[ship].dump_had_storm;
		tot_ship_maelstrom += _data_ship[ship].dump_had_maelstrom;
		tot_ship_loaded += SO_CAPACITY - _data_ship[ship].capacity;
	}

	dprintf(1, "|--SHIPS TOTALS: tot_at_dock: %d, tot_at_sea_with_cargo: %d, tot_at_sea_empty: %d, tot_storm: %d, tot_maeltrom: %d, tot_dead: %d tot_used_capacity %d\n\n",
			tot_ship_dock, tot_ship_cargo, tot_ship_empty, tot_ship_storm, tot_ship_maelstrom, tot_ship_dead, tot_ship_loaded);

	dprintf(1, "[CARGO]\n");
	for (cargo_type = 0; cargo_type < SO_MERCI; cargo_type++){
		dprintf(1, "|----(Cargo type: %d) in_ports: %d tons, in_ships: %d tons, delivered: %d tons, expired_port: %d tons, expired_ship: %d tons, delivered_unwanted: %d tons\n",
				cargo_type, _data_cargo[cargo_type].dump_at_port, _data_cargo[cargo_type].dump_in_ship, _data_cargo[cargo_type].dump_tot_delivered,
				_data_cargo[cargo_type].dump_exipered_port, _data_cargo[cargo_type].dump_exipered_ship, _data_cargo[cargo_type].dump_delivered_unwanted);

		/* Totals */
		tot_cargo_port += _data_cargo[cargo_type].dump_at_port;
		tot_cargo_ship += _data_cargo[cargo_type].dump_in_ship;
		tot_cargo_del += _data_cargo[cargo_type].dump_tot_delivered;
		tot_cargo_exp_port += _data_cargo[cargo_type].dump_exipered_port;
		tot_cargo_exp_ship += _data_cargo[cargo_type].dump_exipered_ship;
		tot_cargo_del_unwanted += _data_cargo[cargo_type].dump_delivered_unwanted;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--CARGO TOTALS: tot_cargo_port: %d, tot_cargo_ship: %d, tot_cargo_delivered: %d, tot_cargo_expired_port: %d, tot_cargo_expired_ship: %d, tot_cargo_delivered_unwanted: %d\n",
			tot_cargo_port, tot_cargo_ship, tot_cargo_del, tot_cargo_exp_port, tot_cargo_exp_ship, tot_cargo_del_unwanted);

#ifdef DEBUG
	/* Shop things */
	dprintf(1, "\n================================[SHOP]====================================\n");
	for (port = 0; port < SO_PORTI; port++){
		dprintf(1, "PORT %d:   ", port);
		for (type = 0; type < SO_MERCI; type++){
			dprintf(1, "(t: %d)%d ", type,
					_data_shop[SO_MERCI * port + type].quantity);
		}
		dprintf(1, "\n");
	}
#endif
	dprintf(1, "================================[END  DAY]================================\n\n\n");
	if(get_day() >= SO_DAYS)
		dprintf(1, "================================[END SIMULATION]================================\n");
}

/* GETTER */
/* Other */
int get_day(){return _data->today;}
bool_t check_ships_all_dead(){
	int i;
	for (i = 0; i < SO_NAVI; i++)
		if(!_data_ship[i].is_dead) return FALSE;

	return TRUE;
}
bool_t check_shop_termination_condition(){
	int port, cargo, amount;
	bool_t never_supply = TRUE, never_demand = TRUE;

	for (port = 0; port < SO_PORTI; port++){
		for (cargo = 0; cargo < SO_MERCI; cargo++){
			amount = DATA_SHOP(port, cargo).quantity;

			never_supply &= !(amount > 0);
			never_demand &= !(amount < 0);
		}
	}

	return never_supply || never_demand;
}
bool_t is_ipc_initialized(){return is_initialized;}
id_shared_t get_id_sem_docks(){return _id_sem_docks;}
id_shared_t get_id_msg_in_ports(){return _id_msg_in_ports;}
id_shared_t get_id_msg_out_ports(){return _id_msg_out_ports;}
/* Port */
struct coord get_port_coord(int port_id){return _data_port[port_id].coordinates;}
int get_port_daily_restock_supply(int port_id){return _data_port[port_id].daily_restock_supply;}
int get_port_daily_restock_demand(int port_id){return _data_port[port_id].daily_restock_demand;}
int get_port_pid(int port_id){return _data_port[port_id].pid;}
int get_port_use(int port_id){return _data_port[port_id].dump_ship_arrived;}
/* Ship */
bool_t is_ship_dead(int ship_id){return _data_ship[ship_id].is_dead;}
struct coord get_ship_coord(int ship_id){return _data_ship[ship_id].coordinates;}
bool_t is_ship_moving(int ship_id){return _data_ship[ship_id].is_moving;}
int get_ship_capacity(int ship_id){return _data_ship[ship_id].capacity;}
int get_ship_pid(int ship_id){return _data_ship[ship_id].pid;}
/* Cargo */
int get_cargo_weight_batch(int cargo_id){return _data_cargo[cargo_id].weight_batch;}
int get_cargo_shelf_life(int cargo_id){return _data_cargo[cargo_id].shelf_life;}
/* Shop */
int get_shop_quantity(int port_id, int cargo_id){return _data_shop[SO_MERCI * port_id + cargo_id].quantity;}

/* "SETTER" */
void start_simulation(){semctl(_id_sem, 0, SETVAL, 0);}
void increase_day(){_data->today++;}
/* Ship */
void set_ship_coord(int ship_id, double x, double y)
{
	_data_ship[ship_id].coordinates.x = x;
	_data_ship[ship_id].coordinates.y = y;
}
void set_ship_dead(int ship_id){_data_ship[ship_id].is_dead = TRUE;}
void set_ship_maelstrom(int ship_id){_data_ship[ship_id].dump_had_maelstrom = TRUE;}
void set_ship_storm(int ship_id){_data_ship[ship_id].dump_had_storm = TRUE;}
void set_ship_pid(int ship_id, pid_t pid){_data_ship[ship_id].pid = pid;}
void set_ship_at_dock(int ship_id, bool_t value, int port_id){
	_data_ship[ship_id].dump_is_at_dock = value;
	if (value) _data_port[port_id].dump_ship_arrived += 1;
}
void set_ship_moving(int ship_id, bool_t value){_data_ship[ship_id].is_moving = value;}
int ship_sell(int ship_id, list_cargo *cargo_hold, int amount, int type){ /* return weight moved*/
	int weight;

	amount = abs(amount);
	remove_cargo(&cargo_hold[type], amount);

	weight = amount * get_cargo_weight_batch(type);
	_data_ship[ship_id].capacity += weight;
	return weight;
}
int ship_buy(int ship_id, list_cargo *cargo_hold, int amount, int type, int expiry_date){ /* Return weight moved */
	int weight;

	amount = abs(amount);
	add_cargo(&cargo_hold[type], amount, expiry_date);

	weight = amount * get_cargo_weight_batch(type);
	_data_ship[ship_id].capacity -= weight;
	return weight;
}
void remove_ship_expired(int ship_id, list_cargo *cargo_hold, int increment_day)
{
	int i, amount_removed;
	for (i = 0; i < SO_MERCI; i++){
		amount_removed = remove_expired_cargo(&cargo_hold[i], get_day() + increment_day);
		_data_ship[ship_id].capacity += amount_removed * _data_cargo[i].weight_batch;

		/* Bump*/
		execute_single_sem_oper(_id_sem_cargo, i, -1);
		_data_cargo[i].dump_in_ship -= amount_removed * get_cargo_weight_batch(i);
		if (increment_day == 0)
			_data_cargo[i].dump_exipered_ship += amount_removed * get_cargo_weight_batch(i);
		else if (increment_day > 0)
			_data_cargo[i].dump_delivered_unwanted += amount_removed * get_cargo_weight_batch(i);
		execute_single_sem_oper(_id_sem_cargo, i, 1);
	}
}
/* Port*/
void set_port_swell(int port_id){_data_port[port_id].dump_had_swell = TRUE;}
void set_port_pid(int port_id, pid_t pid){_data_port[port_id].pid = pid;}

void port_buy(int port_id, int amount, int type)
{
	amount = abs(amount);
	DATA_SHOP(port_id, type).quantity += amount;

	/* Dump */
	execute_single_sem_oper(_id_sem_cargo, type, -1);
	_data_cargo[type].dump_in_ship -= amount * get_cargo_weight_batch(type);
	_data_cargo[type].dump_tot_delivered += amount * get_cargo_weight_batch(type);
	execute_single_sem_oper(_id_sem_cargo, type, 1);
	_data_port[port_id].dump_tot_tons_received += amount * get_cargo_weight_batch(type);
}

int port_sell(int port_id, list_cargo *cargo_hold, int tot_amount, int type, int *expiry_date)
{
	int amount;
	pop_cargo(&cargo_hold[type], &amount, expiry_date);
	if (amount == 0){
		return 0;
	} else if (amount > tot_amount){
		add_cargo(&cargo_hold[type], amount - tot_amount, *expiry_date);
		amount = tot_amount;
	}

	amount = abs(amount);
	DATA_SHOP(port_id, type).quantity -= amount;

	/* Dump */
	execute_single_sem_oper(_id_sem_cargo, type, -1);
	_data_cargo[type].dump_at_port -= amount * get_cargo_weight_batch(type);
	_data_cargo[type].dump_in_ship += amount * get_cargo_weight_batch(type);
	execute_single_sem_oper(_id_sem_cargo, type, 1);
	_data_port[port_id].dump_tot_tons_sent += amount * get_cargo_weight_batch(type);

	/* Return (+expiry date) */
	return amount;
}

void add_port_demand(int port_id, int amount, int type)
{
	DATA_SHOP(port_id, type).quantity -= amount;
}

void add_port_supply(int port_id, list_cargo *cargo_hold, int amount, int type)
{
	DATA_SHOP(port_id, type).quantity += amount;

	add_cargo(&cargo_hold[type], amount,
			get_day() +  get_cargo_shelf_life(type));

	/* Dump */
	execute_single_sem_oper(_id_sem_cargo, type, -1);
	_data_cargo[type].dump_at_port += amount * get_cargo_weight_batch(type);
	execute_single_sem_oper(_id_sem_cargo, type, 1);
}

void remove_port_expired(int port_id, list_cargo *cargo_hold)
{
	int i, amount_removed;
	for (i = 0; i < SO_MERCI; i++){
		amount_removed = remove_expired_cargo(&cargo_hold[i], get_day());
		DATA_SHOP(port_id, i).quantity -= amount_removed;

		/* Bump */
		execute_single_sem_oper(_id_sem_cargo, i, -1);
		_data_cargo[i].dump_at_port -= amount_removed * get_cargo_weight_batch(i);
		_data_cargo[i].dump_exipered_port += amount_removed * get_cargo_weight_batch(i);
		execute_single_sem_oper(_id_sem_cargo, i, 1);
	}
}
