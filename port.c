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
struct int *_data_supply_demand;
struct int *_this_supply_demand;

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
	_data_supply_demand = attach_shared(_data->id_supply_demand);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_port = &_data_port[_this_id];
	_this_supply_demand = &_data_supply_demand[_this_id];

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
	struct commerce_msgbuf response;
	int needed_type = msg_received.cargo_type;
	int needed_supply = msg_received.n_cargo_batch;
	int this_supply = _this_supply_demand[needed_type];

	response = respond_commerce_msgbuf(msg_received);
	response.status = STATUS_REFUSED;

	if (needed_supply < 0  && _this_supply_demand[needed_type] < 0){
		/* If supply is needed respond with how much */
		response.n_cargo_batch = MAX(needed_supply, this_supply);
		response.status = STATUS_ACCEPTED;
	} else (needed_supply > 0 && _this_supply_demand[needed_type] > 0){
		/* If supply is needed respond with how much */
		response.n_cargo_batch = MIN(needed_supply, this_supply);
		response.status = STATUS_ACCEPTED;
	}

	/* Refuse or send message */
	if(response.status == STATUS_REFUSED){
		response.cargo_type = needed_type;
		response.cargo_supply = needed_supply;
	}
	msgsnd(_data->id_msg_out_ports, &response, MSG_SIZE(response), 0);
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
