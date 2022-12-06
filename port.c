#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>
#include "shared_mem.h"
#include "message.h"
#include "semaphore.h"

/* Global variables */
int _this_id;
/* shared memory */
struct const_general *_data;
struct const_port *_data_port;
struct const_port *_this_port;

/* Prototypes */
void supply_demand_update();
void signal_handler(int);
void loop();
void respond_msg(struct commerce_msgbuf);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;
	sigset_t set_masked;
	struct sembuf sem_oper;
	int id;

	/* FIRST: Wait for father */
	id = semget(KEY_SEM, 1, 0600);
	execute_single_sem_oper(id, 0, 0);

	/* FIRST: Gain data struct */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600);
	_data = attach_shared(id);
	_data_port = attach_shared(_data->id_const_port);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_port = &_data_port[_this_id];

	/* LAST: Setting singal handler */
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
	struct commerce_msgbuf msg_received;
	supply_demand_update();
	while (1){
		msgrcv(_data->id_msg_in_ports, &msg_received, MSG_SIZE(msg_received), _this_id, 0);
		/* Check all errors */
		if (errno == EXIT_SUCCESS){
			dprintf(1, "[Child port %d] Received a message\n", getpid());
			respond_msg(msg_received);
		}
	}
}

void respond_msg(struct commerce_msgbuf msg_received)
{
}

void supply_demand_update()
{
}

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGTERM:
		close_all();
	case SIGUSR1: /* Change of day */
		supply_demand_update();
		break;
	case SIGUSR2: /* Seastorm */
		break;
	}
}

void close_all()
{
	/* Detach shared memory */
	detach(_data);
	detach(_data_port);

	dprintf(1, "[Child %d] Fucking dying\n", getpid());
	exit(0);
}
