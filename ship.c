#define _GNU_SOURCE
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/param.h>
#include "shared_mem.h"
#include "message.h"
#include "semaphore.h"
#include "utils.h"


/* Global Variables */
int _this_id;
int _current_capacity;
list_cargo *cargo_hold;
/* shared memory */
struct port *_this_ship;
struct general *_data;
struct port *_data_port;
struct port *_data_ship;
struct cargo *_data_cargo;
int *_data_supply_demand;

/* Prototypes */
void find_destiation_port(int *, double *, double *, int);
void move_to_port(double, double);
void exchange_goods(int);
void signal_handler(int);
void loop();
int sell(int);
int buy(int);
int pick_buy(int, int *, int *);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	int id;
	struct sigaction sa;
	sigset_t set_masked;
	struct sembuf sem_oper;

	dprintf(1, "[Ship] Start initialization\n");
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
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);

	/* LAST: Start running*/
	loop();
}

void loop()
{
	int dest_port, old_port;
	double dest_x, dest_y;

	srand(time(NULL) * getpid()); /* temp */

	old_port = -1;
	dprintf(1, "[Ship %d] Start\n", _this_id);
	while (1){
		find_destiation_port(&dest_port, &dest_x, &dest_y, old_port);
		move_to_port(dest_x, dest_y);
		dprintf(1, "[Ship %d] arrived at %d from %d\n\n", _this_id, dest_port, old_port);
		exchange_goods(dest_port);
		old_port = dest_port;
	}
}

void find_destiation_port(int *dest, double *dest_x, double *dest_y, int old_port)
{
	int offset; /* TODO actually choose */

	if (old_port < 0){ /* not in a port */
		*dest = get_random(0, _data->SO_PORTI);
	}else { /* in port */
		offset = get_random(1, _data->SO_PORTI + 1);
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
	int i, n_batch, n_requested_port, n_batch_ship, tons_moved, weight;

	tons_moved = 0;
	msg = create_commerce_msgbuf(_this_id, port_id);
	for (i = 0; i<_data->SO_MERCI; i++){
		/* Min i have cargo and ports need it */;
		n_requested_port = -_data_supply_demand[port_id * _data->SO_MERCI + i];

		n_batch_ship = count_cargo(&cargo_hold[i]);
		n_batch = MIN(n_batch_ship, n_requested_port);
		/* If nothing to be sell then skip this type*/
		if (n_batch <= 0) continue;

		/* Send message */
		set_commerce_msgbuf(&msg, i, -n_batch, -1, STATUS_REQUEST); /* Not needed expiry date because it is instantly burnt */
		send_commerce_msg(_data->id_msg_in_ports, &msg);

		/* Wait response */
		do {
			receive_commerce_msg(_data->id_msg_out_ports, &msg, _this_id);
		}while(errno == EINTR);

		/* Change data */
		if (msg.status == STATUS_ACCEPTED){
			n_batch = abs(msg.n_cargo_batch);
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
	int n_batch, weight, tons_moved, type;
	struct commerce_msgbuf msg;

	tons_moved = 0;
	msg = create_commerce_msgbuf(_this_id, port_id);
	while (pick_buy(port_id, &type, &n_batch) != -1){
		set_commerce_msgbuf(&msg, type, n_batch, -1, STATUS_REQUEST);
		send_commerce_msg(_data->id_msg_in_ports, &msg);

		/* Wait response */
		do {
			receive_commerce_msg(_data->id_msg_out_ports, &msg, _this_id);
			/* Change data */
			if (errno == EXIT_SUCCESS && msg.status >= STATUS_PARTIAL){
				n_batch = abs(msg.n_cargo_batch);
				add_cargo(&cargo_hold[type], n_batch, msg.expiry_date);
				weight = n_batch * _data_cargo[type].weight_batch;
				tons_moved += weight;
				_current_capacity -= weight;
			}
		}while(errno == EINTR || msg.status == STATUS_PARTIAL);
	}

	return tons_moved;
}

int pick_buy(int port_id, int *pick_type, int *pick_amount)
{
	const int SO_MERCI = _data->SO_MERCI;
	int i, cargo_id, n_cargo, n_cargo_port, n_cargo_capacity;

	/* TODO should start from last time place */
	cargo_id = get_random(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		/* TODO: make it seriously */
		n_cargo_port = _data_supply_demand[port_id * _data->SO_MERCI + cargo_id];
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

void signal_handler(int signal)
{
	switch (signal){
	case SIGTERM:
		close_all();
	case SIGUSR1: /* Storm -> stops the ship for STORM_DURATION time */
		break;
	case SIGUSR2: /* Maeltrom -> sinks all ships in a given range */
		break;
	}
}

void close_all()
{
	/* Local memory deallocation */
	free(cargo_hold);

	/* Detach shared memory */
	detach(_data);
	detach(_data_port);
	detach(_data_ship);
	detach(_data_supply_demand);

	exit(0);
}
