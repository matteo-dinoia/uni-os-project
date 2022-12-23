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

/* Macro*/
#define GET_TRAVEL_TIME(this_ship, dest_x, dest_y, speed)\
	(sqrt(pow((this_ship->x) - (dest_x), 2) + pow((this_ship->y) - (dest_y), 2)) / (double) (speed))

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
void get_next_destination_port(int *, double *, double *);
int new_destiation_port(int);
void discard_expiring_cargo(int);
void move_to_port(double, double);
void exchange_cargo(int);
void signal_handler(int);
void loop();
int sell(int, int);
int buy(int, int, int);
int pick_buy(int, int, int);
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
	_data = attach_shared(id, SHM_RDONLY);
	_data_port = attach_shared(_data->id_port, SHM_RDONLY);
	_data_ship = attach_shared(_data->id_ship, 0);
	_data_supply_demand = attach_shared(_data->id_supply_demand, SHM_RDONLY);
	_data_cargo = attach_shared(_data->id_cargo, 0);

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
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	int dest_port;
	double dest_x, dest_y;

	while (1){
		get_next_destination_port(&dest_port, &dest_x, &dest_y);
		move_to_port(dest_x, dest_y);
		dprintf(1, "\t[Ship %d] arrived at %d\n", _this_id, dest_port);
		exchange_cargo(dest_port);
	}
}

void get_next_destination_port(int *dest, double *dest_x, double *dest_y)
{
	if (_next_port_destination < 0) /* No destination set still */
		new_destiation_port(-1);

	/* Set port */
	*dest = _next_port_destination;
	_next_port_destination = -1;
	/* get position */
	*dest_x = _data_port[*dest].x;
	*dest_y = _data_port[*dest].y;
}

int new_destiation_port(int current_port)
{
	const SO_PORTI = _data->SO_PORTI;
	const SO_MERCI = _data->SO_MERCI;

	int port, cargo_type, sale, not_expired, request_port, min;
	int best_port = -1, best_sale;
	double best_travel_time, travel_time;

	for (port = 0; port < SO_PORTI; port++){
		/* Skip current port */
		if (port == current_port) continue;

		/* Calculate Travel time */
		travel_time = GET_TRAVEL_TIME(_this_ship, _data_port[port].x, _data_port[port].y, _data->SO_SPEED);

		/* Calculate Sale */
		sale = 0;
		for (cargo_type = 0; cargo_type < SO_MERCI; cargo_type++){
			not_expired = get_not_expired_by_day(&cargo_hold[cargo_type],
					_data->today + (int) travel_time);
			request_port = -_data_supply_demand[SO_MERCI * port + cargo_type].quantity;

			min = MIN(not_expired, request_port);
			sale += min > 0 ? min : 0;
		}

		/* Check if is new best */
		if(best_port == -1 /* still doesn't have a best port*/
				|| sale > best_sale /* or best*/
				|| (best_sale == sale && travel_time < best_travel_time)){
			best_port = port;
			best_sale = sale;
			best_travel_time = travel_time;
		}
	}

	return (_next_port_destination = best_port);
}

void discard_expiring_cargo(int dest_id)
{
	const struct port *dest_port = &_data_port[dest_id];

	int days_needed, amount_removed, i;

	/* Time */
	days_needed= GET_TRAVEL_TIME(_this_ship, dest_port->x, dest_port->x, _data->SO_SPEED);

	/* Remove cargo */
	for (i = 0; i < _data->SO_MERCI; i++){
		amount_removed = remove_expired_cargo(&cargo_hold[i], _data->today + days_needed);
		_this_ship->capacity += amount_removed * _data_cargo[i].weight_batch;

		/* Bump*/
		execute_single_sem_oper(_data->id_sem_cargo, i, -1);
		_data_cargo[i].dump_in_ship -= amount_removed;
		_data_cargo[i].dump_exipered_ship += amount_removed;
		execute_single_sem_oper(_data->id_sem_cargo, i, 1);
	}
}

void move_to_port(double x_port, double y_port)
{
	double time = GET_TRAVEL_TIME(_this_ship, x_port, y_port, _data->SO_SPEED);

	/* Wait */
	_this_ship->is_moving = TRUE;
	wait_event_duration(time);
	_this_ship->is_moving = FALSE;

	/* Actual move*/
	_this_ship->x = x_port;
	_this_ship->y = y_port;
}

void exchange_cargo(int port_id)
{
	const SO_MERCI = _data->SO_MERCI;
	struct sembuf sem_buf;
	sigset_t set_masked;
	int tons_moved, i, type, amount, dest_port_id, start_type;

	/* Get dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, -1);
	_this_ship->dump_is_at_dock = TRUE;

	/* Initialize signal ignored */
	sigfillset(&set_masked);

	/* Selling */
	for (type = 0; type < SO_MERCI; type++){
		/* Actual sell (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = sell(port_id, type);
		sigprocmask(SIG_UNBLOCK, &set_masked, NULL);

		/* Wait */
		wait_event_duration(tons_moved / (double)_data->SO_LOADSPEED);
	}

	/* New Dest */
	dest_port_id = new_destiation_port(port_id);
	discard_expiring_cargo(dest_port_id);

	/* Buying */
	start_type = RANDOM(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		/* Pick how much to buy */
		type = (type + i) % SO_MERCI;
		amount = pick_buy(port_id, dest_port_id, type);
		if(amount < 0) continue; /* Cannot buy anything */

		/* Actual buy (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = buy(port_id, type, amount);
		sigprocmask(SIG_BLOCK, &set_masked, NULL);

		/* Wait */
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

	/* Min i have cargo and ports need it */
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



int pick_buy(int port_id, int dest_port_id, int type)
{
	const int SO_MERCI = _data->SO_MERCI;
	int n_sell_this_port, n_capacity, n_buy_dest_port;

	n_sell_this_port = _data_supply_demand[port_id * SO_MERCI + type].quantity;
	n_buy_dest_port = -_data_supply_demand[dest_port_id * SO_MERCI + type].quantity;
	n_capacity = _this_ship->capacity / _data_cargo[type].weight_batch;

	/* Calculate min value */
	return MIN(MIN(n_sell_this_port, n_buy_dest_port), n_capacity);
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
			amount_removed = remove_expired_cargo(&cargo_hold[i], _data->today);
			_this_ship->capacity += amount_removed * _data_cargo[i].weight_batch;

			/* Bump */
			execute_single_sem_oper(_data->id_sem_cargo, i, -1);
			_data_cargo[i].dump_in_ship -= amount_removed;
			_data_cargo[i].dump_exipered_ship += amount_removed;
			execute_single_sem_oper(_data->id_sem_cargo, i, 1);
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
		break;
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
