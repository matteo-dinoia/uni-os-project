#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/param.h>
#include <time.h>
#include "shared_mem.h"
#include "message.h"
#include "semaphore.h"
#include "utils.h"

/* Global variables */
int _this_id;
list_cargo *cargo_hold;
/* shared memory */
struct general *_data;
struct cargo *_data_cargo;
struct port *_data_port;
struct port *_this_port;
int *_data_supply_demand;
int *_this_supply_demand;

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
	_data_port = attach_shared(_data->id_port);
	_data_supply_demand = attach_shared(_data->id_supply_demand);
	_data_cargo = attach_shared(_data->id_cargo);

	/* This*/
	_this_id = atoi(argv[1]);
	_this_port = &_data_port[_this_id];
	_this_supply_demand = &_data_supply_demand[_this_id];
	/* Local memory allocation */
	cargo_hold = calloc(_data->SO_MERCI, sizeof(*cargo_hold));

	/* LAST: Setting singal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);

	/* LAST: Start running*/
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	struct commerce_msgbuf msg;

	supply_demand_update();
	dprintf(1, "[Port %d] Finished shop update\n", _this_id);

	while (1){
		receive_commerce_msg(_data->id_msg_in_ports, &msg, _this_id);
		if (errno == EXIT_SUCCESS)
			respond_msg(msg);
	}
}

void respond_msg(struct commerce_msgbuf msg_received)
{
	struct commerce_msgbuf response;
	int needed_type = msg_received.cargo_type;
	int needed_supply = msg_received.n_cargo_batch;
	int this_supply = _this_supply_demand[needed_type];
	int tot_exchange, amount, expiry_date;

	response = respond_commerce_msgbuf(&msg_received);
	response.status = STATUS_REFUSED;

	if (needed_supply < 0  && _this_supply_demand[needed_type] < 0){
		/* If port is buying respond with how much */
		response.n_cargo_batch = MAX(needed_supply, this_supply);
		response.status = STATUS_ACCEPTED;
		_this_supply_demand[needed_type] -= response.n_cargo_batch;
	} else if (needed_supply > 0 && _this_supply_demand[needed_type] > 0){
		/* If port is selling respond with how much */
		tot_exchange = MIN(needed_supply, this_supply);
		_this_supply_demand[needed_type] -= tot_exchange;

		dprintf(1, "TEST tot %d", tot_exchange);
		while (tot_exchange >= 0){
			/* Spamming messages */
			pop_cargo(&cargo_hold[needed_type], &amount, &expiry_date);
			if (amount > tot_exchange){
				add_cargo(&cargo_hold[needed_type], amount - tot_exchange, expiry_date);
				amount = tot_exchange;
			}
			set_commerce_msgbuf(&response, needed_type, amount, expiry_date, STATUS_PARTIAL);
			tot_exchange -= amount;
			if (tot_exchange <= 0)
				response.status = STATUS_ACCEPTED;
			send_commerce_msg(_data->id_msg_out_ports, &response);
		}
		return;
	}

	/* Refuse or send message */
	if(response.status == STATUS_REFUSED){
		response.cargo_type = needed_type;
		response.n_cargo_batch = needed_supply;
	}
	send_commerce_msg(_data->id_msg_out_ports, &response);
}

void supply_demand_update()
{
	int rem_offer_tons = _this_port->daily_restock_capacity;
	int rem_demand_tons = _this_port->daily_restock_capacity;
	int rand_type;
	bool_t is_demand;

	/* TODO avoid going over the limits */
	while (rem_offer_tons > 0 || rem_demand_tons > 0) {
		rand_type = get_random(0, _data->SO_MERCI);
		if (_this_supply_demand[rand_type] > 0){
			is_demand = FALSE;
		} else if (_this_supply_demand[rand_type] < 0) {
			is_demand = TRUE;
		} else if (rem_offer_tons < rem_demand_tons){
			is_demand = TRUE;
		}else if (rem_offer_tons > rem_demand_tons){
			is_demand = FALSE;
		}else {
			/* TODO fix this shit */
			is_demand = get_random(0, 2);
		}

		if (is_demand){
			if (rem_demand_tons > 0){
				_this_supply_demand[rand_type] -= 1;
				rem_demand_tons -= _data_cargo->weight_batch;
				dprintf(1, "[PP DEMAND on %d] type %d new_quant %d rem %d\n", _this_id, rand_type, _this_supply_demand[rand_type], rem_demand_tons);
			}
		}
		else{
			if (rem_offer_tons > 0){
				_this_supply_demand[rand_type] += 1;
				rem_offer_tons -= _data_cargo->weight_batch;
				dprintf(1, "[PP OFFER on %d] type %d new_quant %d rem %d\n", _this_id, rand_type, _this_supply_demand[rand_type], rem_offer_tons);
			}
		}
	}
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
	/* Local memory deallocation */
	free(cargo_hold);

	/* Detach shared memory */
	detach(_data);
	detach(_data_port);
	detach(_data_supply_demand);

	dprintf(1, "[Child %d] Fucking dying\n", getpid());
	exit(0);
}
