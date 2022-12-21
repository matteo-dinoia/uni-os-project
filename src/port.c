#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/param.h>
#include <time.h>
#include "header/shared_mem.h"
#include "header/message.h"
#include "header/semaphore.h"
#include "header/utils.h"

/* Global variables */
int _this_id;
list_cargo *cargo_hold;
/* shared memory */
struct general *_data;
struct cargo *_data_cargo;
struct port *_data_port;
struct port *_this_port;
struct supply_demand *_data_supply_demand;
struct supply_demand *_this_supply_demand;

/* Prototypes */
void supply_demand_update();
void signal_handler(int);
void loop();
void respond_msg(int, int, int);
void send_to_ship(int, int, int, int, int);
void receive_from_ship(int *, int *, int *, int *, int *);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;
	sigset_t set_masked;
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
	_this_supply_demand = &_data_supply_demand[_this_id * _data->SO_MERCI];
	/* Local memory allocation */
	cargo_hold = calloc(_data->SO_MERCI, sizeof(*cargo_hold));

	/* LAST: Setting singal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;
	sigaction(SIGDAY, &sa, NULL);
	sigaction(SIGSWELL, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* LAST: Start running*/
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	int ship_id, needed_type, needed_amount;
	supply_demand_update();

	while (1){
		receive_from_ship(&ship_id, &needed_type, &needed_amount, NULL, NULL);
		respond_msg(ship_id, needed_type, needed_amount);
	}
}

void respond_msg(int ship_id, int needed_type, int needed_amount)
{
	const int this_amount = _this_supply_demand[needed_type].quantity;

	int tot_exchange = 0, amount = 0, expiry_date = 0;
	int status = STATUS_REFUSED;

	if (needed_amount > 0 && this_amount > 0){
		/* If port is selling respond with how much */
		tot_exchange = MIN(needed_amount, this_amount);
		_this_supply_demand[needed_type].quantity -= tot_exchange;

		while (tot_exchange > 0){
			/* Spamming messages */
			pop_cargo(&cargo_hold[needed_type], &amount, &expiry_date);
			if(amount == 0){
				status = STATUS_REFUSED;
				break;
			}else if (amount > tot_exchange){
				add_cargo(&cargo_hold[needed_type], amount - tot_exchange, expiry_date);
				amount = tot_exchange;
			}

			tot_exchange -= amount;
			TEST();
			status = tot_exchange <= 0 ? STATUS_ACCEPTED : STATUS_PARTIAL;
			send_to_ship(ship_id, needed_type, amount, expiry_date, status);
		}
		if(tot_exchange <= 0)
			return;
	}else if (needed_amount < 0  && this_amount < 0){
		/* If port is buying respond with how much */
		amount = MAX(needed_amount, this_amount);
		_this_supply_demand[needed_type].quantity -= amount;
		status = STATUS_ACCEPTED;
	}

	/* Refuse or send message */
	send_to_ship(ship_id, needed_type, amount, expiry_date, status);
}

void send_to_ship(int ship_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, ship_id,
			cargo_type, amount, expiry_date, status);

	dprintf(1, "PORT %d SEND TO SHIP %d\n", _this_id, ship_id);
	send_commerce_msg(_data->id_msg_out_ports, &msg);
}

void receive_from_ship(int *ship_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	dprintf(1, "PORT %d LISTEN TO SHIPS\n", _this_id);
	receive_commerce_msg(_data->id_msg_in_ports, _this_id,
			ship_id, cargo_type, amount, expiry_date, status);
	dprintf(1, "PORT %d RECEIVED FROM SHIPS\n", _this_id);
}

void supply_demand_update()
{
	int rem_offer_tons = _this_port->daily_restock_capacity;
	int rem_demand_tons = _this_port->daily_restock_capacity;
	int rand_type, sum_normalized_first_two;
	bool_t is_demand;

	/* TODO avoid going over the limits */
	while (rem_offer_tons > 0 || rem_demand_tons > 0) {
		rand_type = RANDOM(0, _data->SO_MERCI);
		if (_this_supply_demand[rand_type].quantity > 0){
			is_demand = FALSE;
		}else if (_this_supply_demand[rand_type].quantity < 0) {
			is_demand = TRUE;
		}else if (rem_offer_tons < rem_demand_tons){
			is_demand = TRUE;
		}else if (rem_offer_tons > rem_demand_tons){
			is_demand = FALSE;
		}else {
			/* TODO clear this shit */
			sum_normalized_first_two = GET_SIGN(_this_supply_demand[0].quantity)
					+ GET_SIGN(_this_supply_demand[0].quantity);
			if (sum_normalized_first_two > 0){
				is_demand = TRUE;
			}else if (sum_normalized_first_two < 0){
				is_demand = FALSE;
			}else is_demand = RANDOM(0, 2);
		}

		if (is_demand && rem_demand_tons > 0){
			_this_supply_demand[rand_type].quantity -= 1;
			rem_demand_tons -= _data_cargo->weight_batch;
		}
		else if (!is_demand && rem_offer_tons > 0){
			_this_supply_demand[rand_type].quantity += 1;
			rem_offer_tons -= _data_cargo->weight_batch;
			add_cargo(&cargo_hold[rand_type], _data_cargo->weight_batch,
					_data->today + _data_cargo->shelf_life);
		}
	}
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;

	switch (signal)
	{
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In port (closing)");
	case SIGINT:
		close_all();
		break;
	case SIGDAY: /* Change of day */
		supply_demand_update();
		break;
	case SIGSWELL: /* Swell */
		wait_time = get_timespec(_data->SO_SWELL_DURATION/24.0);
		do {
			errno = EXIT_SUCCESS;
			nanosleep(&wait_time, &rem_time);
			wait_time = rem_time;
		} while (errno == EINTR);
		_this_port->dump_had_swell = TRUE;
		break; /* TODO: receive possile second signal equal */
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

	exit(0);
}
