#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/sem.h>
#include "shared_mem.h"
#include "message.h"
#include "semaphore.h"
#include "utils.h"
#include <sys/param.h>

/* Global Variables */
int _this_id;
list_cargo *cargo_hold;
/* shared memory */
struct const_port *_this_ship;
struct const_general *_data;
struct const_port *_data_port;
struct const_port *_data_ship;
int *_data_supply_demand;

/* Prototypes */
void find_destiation_port(int *, double *, double *);
void move_to_port(double, double);
void exchange_goods(int);
void signal_handler(int);
void loop();
int sell(int port_id);
int buy(int port_id);
void close_all();

int main(int argc, char *argv[])
{
	/* Variables */
	int id;
	struct sigaction sa;
	sigset_t set_masked;
	struct sembuf sem_oper;

	/* FIRST: Wait for father */
	id = semget(KEY_SEM, 1, 0600);
	execute_single_sem_oper(id, 0, 0);

	/* FIRST: Gain data struct */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600);
	_data = attach_shared(id);
	_data_port = attach_shared(_data->id_const_port);
	_data_ship = attach_shared(_data->id_const_ship);
	_data_supply_demand = attach_shared(_data->id_supply_demand);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_ship = &_data_ship[_this_id];

	/* Local memory allocation */
	cargo_hold = calloc(_data->SO_MERCI, sizeof(*cargo_hold));
	bzero(cargo_hold, _data->SO_MERCI * sizeof(*cargo_hold));

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
	int dest_port;
	double dest_x, dest_y;

	srand(time(NULL) * getpid()); /* temp */

	dprintf(1, "[Child ship %d] Start looping\n", getpid());
	while (1){
		find_destiation_port(&dest_port, &dest_x, &dest_y);
		move_to_port(dest_x, dest_y);
		exchange_goods(dest_port);
	}
}

void find_destiation_port(int *dest, double *dest_x, double *dest_y)
{
	/* TODO actually choose */
	*dest = rand() % _data->SO_PORTI;

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

int sell(int port_id){
	struct commerce_msgbuf msg;
	int i, n_batch, n_requested_port, n_batch_ship, tons_moved;

	tons_moved = 0;
	msg = create_commerce_msgbuf(_this_id, port_id);
	for (i; i<_data->SO_MERCI; i++){
		/* Min i have cargo and ports need it */
		n_requested_port = -_data_supply_demand[port_id * _data->SO_MERCI + i];
		n_batch_ship = count_cargo(&cargo_hold[i]);
		n_batch = MIN(n_batch_ship, n_requested_port);
		/* If nothing to be sell then skip this type*/
		if (n_batch <= 0) continue;

		/* Send message */
		set_commerce_msgbuf(&msg, i, -n_batch, -1, STATUS_REQUEST);
		msgsnd(_data->id_msg_in_ports, &msg, MSG_SIZE(msg), 0);

		/* Wait response */
		do {
			msgrcv(_data->id_msg_out_ports, &msg, MSG_SIZE(msg), _this_id, 0);
		}while(errno == EXIT_FAILURE);

		/* Change data */
		if (msg.status == STATUS_ACCEPTED){
			n_batch = abs(msg.n_cargo_batch);
			remove_cargo(&cargo_hold[i], n_batch);
			tons_moved += n_batch;
		}

	}

	return tons_moved;
}

int buy(int port_id){
	/* TODO implements */
	return -1;
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

	exit(0);
}
