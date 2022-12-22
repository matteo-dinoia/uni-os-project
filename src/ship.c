#define _GNU_SOURCE
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/param.h>
#include "header/shared_mem.h"
#include "header/message.h"
#include "header/semaphore.h"
#include "header/utils.h"


/* Global Variables */
int _this_id;
int _next_port_destination;
list_cargo *cargo_hold;
/* shared memory */
struct ship *_this_ship;
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;
struct cargo *_data_cargo;
struct supply_demand *_data_supply_demand;

/* Prototypes */
void get_next_destination_port(int, int *, double *, double *);
int new_destiation_port(int);
void move_to_port(double, double);
void exchange_goods(int);
void signal_handler(int);
void loop();
int sell(int, int);
int buy(int, int, int);
int pick_buy(int, int, int *, int *);
void send_to_port(int, int, int, int, int);
void receive_from_port(int *, int *, int *, int *, int *);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	int id;
	struct sigaction sa;
	sigset_t set_masked;

	/* FIRST: Wait for father */
	id = semget(KEY_SEM, 1, 0600);
	execute_single_sem_oper(id, 0, 0);

	/* FIRST: Gain data struct */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600);
	_data = attach_shared(id);
	_data_port = attach_shared(_data->id_port);
	_data_ship = attach_shared(_data->id_ship);
	_data_supply_demand = attach_shared(_data->id_supply_demand);
	_data_cargo = attach_shared(_data->id_cargo);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_ship = &_data_ship[_this_id];

	/* Local memory allocation */
	cargo_hold = calloc(_data->SO_MERCI, sizeof(*cargo_hold));

	/* LAST: Setting signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;
	sigaction(SIGSTORM, &sa, NULL);
	sigaction(SIGMAELSTROM, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* LAST: Start running*/
	srand(time(NULL) * getpid()); /* TODO temp */
	loop();
}

void loop()
{
	int dest_port, old_port;
	double dest_x, dest_y;

	old_port = -1;
	while (1){
		get_next_destination_port(old_port, &dest_port, &dest_x, &dest_y);
		move_to_port(dest_x, dest_y);
		dprintf(1, "\t[Ship %d] arrived at %d from %d\n", _this_id, dest_port, old_port);
		exchange_goods(dest_port);
		old_port = dest_port;
	}
}

void get_next_destination_port(int current_port_id, int *dest, double *dest_x, double *dest_y)
{
	if (_next_port_destination < 0) /* No destination set still */
		new_destiation_port(current_port_id);

	/* Set port */
	*dest = _next_port_destination;
	_next_port_destination = -1;
	/* get position */
	*dest_x = _data_port[*dest].x;
	*dest_y = _data_port[*dest].y;
}

int new_destiation_port(int current_port_id)
{
	int offset; /* TODO actually choose */

	if (current_port_id < 0){ /* not in a port */
		_next_port_destination = RANDOM(0, _data->SO_PORTI);
	}else { /* in port */
		offset = RANDOM(1, _data->SO_PORTI);
		_next_port_destination = (current_port_id + offset) % _data->SO_PORTI;
	}

	return _next_port_destination; /* TODO remove cargo that will expire anyway (+dump)*/
}

void move_to_port(double x_port, double y_port)
{
	const int x = _this_ship->x;
	const int y = _this_ship->y;
	double distance;

	/* Distance */
	distance = sqrt(pow((x - x_port), 2) + pow((y - y_port), 2));

	/* Wait */
	_this_ship->is_moving = TRUE;
	wait_event_duration(distance / _data->SO_SPEED);
	_this_ship->is_moving = FALSE;

	/* Actual move*/
	_this_ship->x = x_port;
	_this_ship->y = y_port;
}

void exchange_goods(int port_id)
{
	/* TODO make ship unsinkable while trading */
	struct sembuf sem_buf;
	int tons_moved, i, type, amount, dest_port_id;

	/* Get dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, -1);
	_this_ship->dump_is_at_dock = TRUE;

	/* Selling */
	for (type = 0; type < _data->SO_MERCI; type++){
		tons_moved = sell(port_id, type);
		wait_event_duration(tons_moved / (double)_data->SO_LOADSPEED);
	}

	/* Buying */
	dest_port_id = new_destiation_port(port_id);
	for (i = 0; i < _data->SO_MERCI; i++){
		if(pick_buy(port_id, dest_port_id, &type, &amount) == -1)
			break; /* Cannot buy anything */

		tons_moved = buy(port_id, type, amount);
		wait_event_duration(tons_moved / (double)_data->SO_LOADSPEED);
	}

	/* Free dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, 1);
	_this_ship->dump_is_at_dock = FALSE;
}

int sell(int port_id, int type_to_sell)
{
	int amount, amount_port, amount_ship;
	int i, weight, status;


	/* Min i have cargo and ports need it */;
	amount_port = -_data_supply_demand[port_id * _data->SO_MERCI + i].quantity;
	amount_ship = count_cargo(&cargo_hold[i]);
	amount = MIN(amount_ship, amount_port);
	/* If nothing to be sell then skip this type*/
	if (amount <= 0) return 0;

	/* Send and receive message */
	send_to_port(port_id, i, -amount, -1, STATUS_REQUEST);
	receive_from_port(NULL, NULL, &amount, NULL, &status);

	/* Change data */
	if (status == STATUS_ACCEPTED && amount < 0){
		amount = abs(amount);
		remove_cargo(&cargo_hold[i], amount);
		weight = amount * _data_cargo[i].weight_batch;
		_this_ship->capacity += weight;

		return weight;
	}

	return 0;
}

int buy(int port_id, int type_to_buy, int amount_to_buy)
{
	int amount, weight, tons_moved, type, expiry_date, status;

	/* Send request*/
	send_to_port(port_id, type_to_buy, amount_to_buy, -1, STATUS_REQUEST);

	/* Wait response */
	tons_moved = 0;
	do {
		receive_from_port(NULL, &type, &amount, &expiry_date, &status);
		/* Change data */
		if (status == STATUS_PARTIAL || status == STATUS_ACCEPTED){
			add_cargo(&cargo_hold[type], amount, expiry_date);
			weight = amount * _data_cargo[type].weight_batch;
			tons_moved += weight;
			_this_ship->capacity -= weight;
		}
	}while(status == STATUS_PARTIAL);

	return tons_moved;
}



int pick_buy(int port_id, int dest_port_id, int *pick_type, int *pick_amount)
{
	const int SO_MERCI = _data->SO_MERCI;
	int i, cargo_id, n_cargo, n_cargo_port, n_cargo_capacity;

	/* TODO should not be random */
	cargo_id = RANDOM(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		/* TODO: make it seriously */
		n_cargo_port = _data_supply_demand[port_id * _data->SO_MERCI + cargo_id].quantity;
		n_cargo_capacity = _this_ship->capacity / _data_cargo[i].weight_batch;
		n_cargo = MIN(n_cargo_port, n_cargo_capacity);

		if (n_cargo > 0){
			*pick_type = cargo_id;
			*pick_amount = n_cargo;
			return 0;
		}

		cargo_id = (cargo_id + 1) % SO_MERCI;
	}

	return -1; /* If nothing to buy is found */
}

void send_to_port(int port_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, port_id,
			cargo_type, amount, expiry_date, status);

	dprintf(1, "SHIP %d SEND TO PORT %d\n", _this_id, port_id);
	send_commerce_msg(_data->id_msg_in_ports, &msg);
}

void receive_from_port(int *port_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	dprintf(1, "SHIP %d LISTEN TO PORTS\n", _this_id);
	receive_commerce_msg(_data->id_msg_out_ports, _this_id,
			port_id, cargo_type, amount, expiry_date, status);
	dprintf(1, "SHIP %d RECEIVED FROM PORTS status %d amount %d\n", _this_id, *status, *amount);
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;
	int i, amount_removed;

	switch (signal){
	case SIGDAY:
		for (i = 0; i < _data->SO_MERCI; i++){
			amount_removed = remove_expired_cargo(&cargo_hold, _data->today);
			execute_single_sem_oper(_data->id_sem_cargo, i, -1);
			_data_cargo[i].dump_in_ship -= amount_removed;
			_data_cargo[i].dump_exipered_ship += amount_removed;
			execute_single_sem_oper(_data->id_sem_cargo, i, 1);
			_this_ship->capacity += amount_removed * _data_cargo[i].weight_batch;
		}
		break;
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In ship (closing)");
		close_all();
		break;
	case SIGMAELSTROM: /* Maeltrom -> sinks the ship */
		_this_ship->dump_had_maelstrom = TRUE;
	case SIGINT: /* Closing for every other reason */
		close_all();
		break;
	case SIGSTORM: /* Storm -> stops the ship for STORM_DURATION time */
		wait_time = get_timespec(_data->SO_STORM_DURATION/24.0);
		do {
			nanosleep(&wait_time, &rem_time);
			wait_time = rem_time;
		} while (errno == EINTR);
		_this_ship->dump_had_storm = TRUE;
		break; /* TODO: receive possile second signal equal */
	}
}

void close_all()
{
	int i, amount;

	/* Signaling data of death */
	_this_ship->pid = 0;

	/* Local memory deallocation */
	for (i = 0; i < _data->SO_MERCI; i++){
		amount = count_cargo(&cargo_hold[i]);
		free_cargo(&cargo_hold[i]);

		/* Dump */
		execute_single_sem_oper(_data->id_sem_cargo, i, -1);
		_data_cargo[i].dump_in_ship -= amount;
		_data_cargo[i].dump_exipered_ship += amount;
		execute_single_sem_oper(_data->id_sem_cargo, i, 1);
	}
	free(cargo_hold);

	/* Detach shared memory */
	detach(_data);
	detach(_data_port);
	detach(_data_ship);
	detach(_data_supply_demand);

	exit(0);
}
