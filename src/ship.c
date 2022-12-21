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
int _current_capacity;
list_cargo *cargo_hold;
/* shared memory */
struct ship *_this_ship;
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;
struct cargo *_data_cargo;
struct supply_demand *_data_supply_demand;

/* Prototypes */
void find_destiation_port(int *, double *, double *, int);
void move_to_port(double, double);
void exchange_goods(int);
void signal_handler(int);
void loop();
int sell(int);
int buy(int);
int pick_buy(int, int *, int *);
void send_to_port(int port_id, int cargo_type, int amount, int expiry_date, int status);
void receive_from_port(int *port_id, int *cargo_type, int *amount, int *expiry_date, int *status);
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
	_current_capacity = _data->SO_CAPACITY;

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
		find_destiation_port(&dest_port, &dest_x, &dest_y, old_port);
		move_to_port(dest_x, dest_y);
		dprintf(1, "\t[Ship %d] arrived at %d from %d\n", _this_id, dest_port, old_port);
		exchange_goods(dest_port);
		old_port = dest_port;
	}
}

void find_destiation_port(int *dest, double *dest_x, double *dest_y, int old_port)
{
	int offset; /* TODO actually choose */

	if (old_port < 0){ /* not in a port */
		*dest = RANDOM(0, _data->SO_PORTI);
	}else { /* in port */
		offset = RANDOM(1, _data->SO_PORTI);
		*dest = (old_port + offset) % _data->SO_PORTI;
	}

	/* get position */
	*dest_x = _data_port[*dest].x;
	*dest_y = _data_port[*dest].y;
}

void move_to_port(double x_port, double y_port)
{
	struct timespec rem_time, travel_time;
	const int x = _this_ship->x;
	const int y = _this_ship->y;
	double distance;

	/* Time */
	distance = sqrt(pow((x - x_port), 2) + pow((y - y_port), 2));
	travel_time = get_timespec(distance / _data->SO_SPEED);

	/* Wait */
	do {
		nanosleep(&travel_time, &rem_time);
		travel_time = rem_time;
	} while (errno == EINTR);

	/* Actual move*/
	_this_ship->x = x_port;
	_this_ship->y = y_port;
}

void exchange_goods(int port_id)
{
	struct sembuf sem_buf;
	struct timespec rem_time, commerce_time;
	int tot_tons_moved;

	/* Get dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, -1);

	/* Communicate selling and buying request */
	tot_tons_moved = sell(port_id) + buy(port_id);

	/* Wait for every transaction to be "made" */
	commerce_time = get_timespec(tot_tons_moved / (double)_data->SO_LOADSPEED);
	do {
		nanosleep(&commerce_time, &rem_time);
		commerce_time = rem_time;
	} while (errno == EINTR);

	/* Free dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, 1);
}

int sell(int port_id)
{
	struct commerce_msgbuf msg;
	int n_batch, n_requested_port, n_batch_ship;
	int i, tons_moved, weight, status;

	tons_moved = 0;

	for (i = 0; i<_data->SO_MERCI; i++){
		/* Min i have cargo and ports need it */;
		n_requested_port = -_data_supply_demand[port_id * _data->SO_MERCI + i].quantity;

		n_batch_ship = count_cargo(&cargo_hold[i]);
		n_batch = MIN(n_batch_ship, n_requested_port);
		/* If nothing to be sell then skip this type*/
		if (n_batch <= 0) continue;

		/* Send message */
		send_to_port(port_id, i, -n_batch, -1, STATUS_REQUEST);

		/* Wait response */
		receive_from_port(NULL, NULL, &n_batch, NULL, &status);

		/* Change data */
		if (status == STATUS_ACCEPTED && n_batch < 0){
			n_batch = abs(n_batch);
			remove_cargo(&cargo_hold[i], n_batch);
			weight = n_batch * _data_cargo[i].weight_batch;
			tons_moved += weight;
			_current_capacity += weight;
		}

	}

	return tons_moved;
}

int buy(int port_id)
{
	int amount, weight, tons_moved, type, expiry_date, status;

	tons_moved = 0;
	while (pick_buy(port_id, &type, &amount) != -1){
		send_to_port(port_id, type, amount, -1, STATUS_REQUEST);

		/* Wait response */
		do {
			receive_from_port(NULL, &type, &amount, &expiry_date, &status);
			/* Change data */
			if (status == STATUS_PARTIAL || status == STATUS_ACCEPTED){
				add_cargo(&cargo_hold[type], amount, expiry_date);
				weight = amount * _data_cargo[type].weight_batch;
				tons_moved += weight;
				_current_capacity -= weight;
			}
		}while(status == STATUS_PARTIAL);
	}

	return tons_moved;
}

int pick_buy(int port_id, int *pick_type, int *pick_amount)
{
	const int SO_MERCI = _data->SO_MERCI;
	int i, cargo_id, n_cargo, n_cargo_port, n_cargo_capacity;

	/* TODO should start from last time place */
	cargo_id = RANDOM(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		/* TODO: make it seriously */
		n_cargo_port = _data_supply_demand[port_id * _data->SO_MERCI + cargo_id].quantity;
		n_cargo_capacity = _current_capacity / _data_cargo[i].weight_batch;
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
	dprintf(1, "SHIP %d RECEIVED FROM PORTS status %d\n", _this_id, *status);
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;

	switch (signal){
	case SIGINT: /* Closing for every other reason */
	case SIGMAELSTROM: /* Maeltrom -> sinks the ship */
		_this_ship->dump_had_maeltrom = TRUE;
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
	/* Signaling data of death */
	_this_ship->pid = 0;
	/* TODO: dump*/

	/* Local memory deallocation */
	free(cargo_hold);

	/* Detach shared memory */
	detach(_data);
	detach(_data_port);
	detach(_data_ship);
	detach(_data_supply_demand);

	exit(0);
}
