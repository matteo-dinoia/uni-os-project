#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include "shared_mem.h"

/* Global variables */
/* shared memory */
struct const_general *_data;
struct const_port *_data_port;
struct const_port *_this_port;

/* Prototypes */
void supply_demand_update();
void signal_handler(int);
void loop();

int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;
	struct sembuf sem_oper;
	int id_sem, id_shm, this_id;

	/* Wait for father */
	id_sem = semget(KEY_SHARED, 1, 0600);
	sem_oper = create_sembuf(0, 0);
	semop(id_sem, &sem_oper, 1);

	/* Gain data struct */
	id_shm = get_shared(KEY_SHARED, sizeof(*_data));
	_data = attach_shared(id_shm);
	_data_port = attach_shared(_data->id_const_port);

	/* Setting singal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	/* This */
	this_id = *(int *)argv[1];
	_this_port = &_data_port[this_id];

	loop();
}

void loop(){
	supply_demand_update();
	while(1){
		pause();
	}
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
