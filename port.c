#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include "base.h"

/* Global variables */
int this_id;
/* shared memory */
struct const_general *_data;

/* Prototypes */
void supply_demand_update();
void signal_handler(int);

int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;
	struct sembuf sem_oper;
	int id_sem, id_shm;

	/* Wait for father */
	id_sem = semget(KEY_SHARED, 1, 0600);
	sem_oper = create_sembuf(0, 0);
	semop(id_sem, &sem_oper, 1);

	/* Gain data struct */
	id_shm = shmget(KEY_SHARED, sizeof(*_data), 0600);
	_data = shmat(id_shm, NULL, 0);

	/* Setting singal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	/* TODO check for invalid arguments */
	this_id = atoi(argv[1]); /* Still needed*/

	supply_demand_update();
}

void supply_demand_update()
{
}

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGUSR1: /* Change of day */
		supply_demand_update();
		break;
	case SIGUSR2: /* Seastorm */
		break;
	}
}
