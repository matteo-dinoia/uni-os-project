#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/sem.h>
#include "base.h"

/* Global Variables */
struct simulation_constant *_constants;

/* Prototypes */
int find_destiation_port(double, double);
void move_to_port(int, double, double);
void exchange_goods(int);
void signal_handler(int);

int main(int argc, char *argv[])
{
	/* Variables */
	double x, y; /*  Coordinates */
	int dest_port;
	int id_sem, id_shm;
	struct sigaction sa;
	struct sembuf sem_operation;

	/* Wait for father */
	id_sem=semget(KEY_SHARED, 1, 0600);
	sem_operation.sem_num=0;
	sem_operation.sem_op=0;
	sem_operation.sem_flg=0;
	semop(id_sem, &sem_operation, 1);

	/* Gain data struct */
	id_shm=shmget(KEY_SHARED, sizeof(*_constants), 0600);
	_constants=shmat(id_shm, NULL, 0);

	/* Set signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);


	/* TODO check for invalid arguments */
	x = atof(argv[1]);
	y = atof(argv[2]);
	while (1) {
		dest_port = find_destiation_port(x, y);
		move_to_port(dest_port, x , y);
		exchange_goods(dest_port);
	}
}

int find_destiation_port(double x, double y)
{

	return -1;
}

void move_to_port(int port_id, double x, double y)
{
	struct timespec rem_time;
	struct timespec travel_time;
	double x_port = 0, y_port = 0; /* TODO need to get from data structure */
	travel_time.tv_nsec = sqrt(pow((x-x_port), 2) + pow((y-y_port), 2));

	do {
		nanosleep(&travel_time, &rem_time);
		travel_time = rem_time;
	} while(errno == EINTR);
}

void exchange_goods(int port_id)
{

}

void signal_handler(int signal)
{
	switch (signal) {
	case SIGUSR1: /* Storm -> stops the ship for STORM_DURATION time */
		break;
	case SIGUSR2: /* Maeltrom -> sinks all ships in a given range */
		break;
	}
}
