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

/* Global Variables */
int _this_id;
/* shared memory */
struct const_port *_this_ship;
struct const_general *_data;
struct const_port *_data_port;
struct const_port *_data_ship;
struct int *_data_supply_demand;

/* Prototypes */
void find_destiation_port(int *, double *, double *);
void move_to_port(double, double);
void exchange_goods(int);
void signal_handler(int);
void loop();
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
	_data_supply_demand = attach_shared(_data->id_suppy_demand);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_ship = &_data_ship[_this_id];

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
	*dest = rand() % _data->SO_NAVI;

	/* get position */
	*dest_x = _data_port[*dest].x;
	*dest_y = _data_port[*dest].y;
}

void move_to_port(double x_port, double y_port)
{
	struct timespec rem_time, travel_time;
	const int x = _this_ship->x;
	const int y = _this_ship->y;
	double distance, time_days;

	/* Time */
	distance = sqrt(pow((x - x_port), 2) + pow((y - y_port), 2));
	time_days = distance / _data->SO_SPEED;
	travel_time.tv_sec = (long)time_days;
	travel_time.tv_nsec = (time_days - (long)time_days) * pow(10, 9);


	/* Wait */
	dprintf(1, "[Child ship %d] Time =%lf, %ld.%09ld\n", getpid(), time_days, travel_time.tv_sec, travel_time.tv_nsec);
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
	struct commerce_msgbuf msg;
	struct sembuf sem_buf;
	int i, n_batch, n_requested_port, n_batch_ship;

	/* Get dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, -1);

	/* Communicate selling request */
	for (i; i<_data->SO_MERCI; i++){
		n_requested_port = -_data_supply_demand[port_id * _data->SO_MERCI + i];
		if (n_requested_port <= 0) continue; /* Only care of port request */

		n_batch_ship /* TODO = */;

		/* min i have cargo and ports need it*/
		n_batch = MIN(n_batch_ship, n_requested_port);
		if (n_batch != 0){
			msg.n_cargo_batch = -n_batch;
			msg.cargo_type = i;
			msg.status = STATUS_REQUEST;

			msg = create_commerce_msgbuf(_this_id, port_id);
			msgsnd(_data->id_msg_in_ports, &msg, MSG_SIZE(msg), 0);

			do {
				msgrcv(_data->id_msg_out_ports, &msg, MSG_SIZE(msg), _this_id, 0);
			}while(errno == EXIT_FAILURE);

			/* TODO change data */
		}
	}

	/* Communicate buying request */

	/* Wait for every translation to be "made" */

	/* Free dock */
	execute_single_sem_oper(_data->id_sem_docks, port_id, 1);
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
	/* Detach shared memory */
	detach(_data);
	detach(_data_port);
	detach(_data_ship);

	exit(0);
}
