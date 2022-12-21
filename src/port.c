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
void respond_msg(struct commerce_msgbuf);
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

	/* LAST: Start running*/
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	struct commerce_msgbuf msg;

	supply_demand_update();

	while (1){
		receive_commerce_msg(_data->id_msg_in_ports, &msg, _this_id);
		if (errno == EXIT_SUCCESS)
			respond_msg(msg);
	}
}

void respond_msg(struct commerce_msgbuf msg_received)
{
	const int needed_type = msg_received.cargo_type;
	const int needed_supply = msg_received.n_cargo_batch;
	const int this_supply = _this_supply_demand[needed_type].quantity;

	struct commerce_msgbuf response;
	int tot_exchange, amount, expiry_date;

	response = respond_commerce_msgbuf(&msg_received);
	response.status = STATUS_REFUSED;

	dprintf(1, "!!!1-TEST  need %d have %d or %d!!!\n", needed_supply, this_supply, _data_supply_demand[_this_id * _data->SO_MERCI + needed_type].quantity);

	if (needed_supply < 0  && this_supply < 0){
		/* If port is buying respond with how much */
		response.n_cargo_batch = MAX(needed_supply, this_supply);
		response.status = STATUS_ACCEPTED;
		_this_supply_demand[needed_type].quantity -= response.n_cargo_batch;
	} else if (needed_supply > 0 && this_supply > 0){
		/* If port is selling respond with how much */
		tot_exchange = MIN(needed_supply, this_supply);
		_this_supply_demand[needed_type].quantity -= tot_exchange;

		while (tot_exchange > 0){
			/* Spamming messages */
			pop_cargo(&cargo_hold[needed_type], &amount, &expiry_date);
			if(amount == 0){
				dprintf(1, "!---[ERROR] Port is responding with 0\n");
				set_commerce_msgbuf(&response, needed_type, amount, expiry_date, STATUS_ACCEPTED);
				do{
					send_commerce_msg(_data->id_msg_out_ports, &response);
				}while(errno == EINTR);
				return;
			}
			if (amount > tot_exchange){
				add_cargo(&cargo_hold[needed_type], amount - tot_exchange, expiry_date);
				amount = tot_exchange;
			}
			set_commerce_msgbuf(&response, needed_type, amount, expiry_date, STATUS_PARTIAL);
			tot_exchange -= amount;
			if (tot_exchange <= 0)
				response.status = STATUS_ACCEPTED;
			do{
				send_commerce_msg(_data->id_msg_out_ports, &response);
			}while(errno == EINTR);

			if(tot_exchange <= 0)
				return;
		}
	} else response.status == STATUS_REFUSED;

	/* Refuse or send message */
	if(response.status == STATUS_REFUSED){
		set_commerce_msgbuf(&response, needed_type, amount, expiry_date, STATUS_REFUSED);
	}
	do{
		send_commerce_msg(_data->id_msg_out_ports, &response);
	}while(errno == EINTR);
}

void supply_demand_update()
{
	int rem_offer_tons = _this_port->daily_restock_capacity;
	int rem_demand_tons = _this_port->daily_restock_capacity;
	int rand_type;
	bool_t is_demand;

	/* TODO avoid going over the limits */
	while (rem_offer_tons > 0 || rem_demand_tons > 0) {
		rand_type = RANDOM(0, _data->SO_MERCI);
		if (_this_supply_demand[rand_type].quantity > 0){
			is_demand = FALSE;
		} else if (_this_supply_demand[rand_type].quantity < 0) {
			is_demand = TRUE;
		} else if (rem_offer_tons < rem_demand_tons){
			is_demand = TRUE;
		}else if (rem_offer_tons > rem_demand_tons){
			is_demand = FALSE;
		}else {
			/* TODO fix this shit */
			is_demand = RANDOM(0, 2);
		}

		if (is_demand && rem_demand_tons > 0){
			_this_supply_demand[rand_type].quantity -= 1;
			rem_demand_tons -= _data_cargo->weight_batch;
			dprintf(1, "[PP DEMAND on %d] type %d new_quant %d rem %d\n", _this_id + 1, rand_type, _this_supply_demand[rand_type].quantity, rem_demand_tons);
		}
		else if (!is_demand && rem_offer_tons > 0){
			_this_supply_demand[rand_type].quantity += 1;
			rem_offer_tons -= _data_cargo->weight_batch;
			add_cargo(cargo_hold, _data_cargo->weight_batch, RANDOM(9000, 9999)); /* TODO expiry date*/
			dprintf(1, "[PP OFFER on %d] type %d new_quant %d rem %d\n", _this_id + 1, rand_type, _this_supply_demand[rand_type].quantity, rem_offer_tons);
		}
	}
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;

	switch (signal)
	{
	case SIGINT:
		close_all();
	case SIGDAY: /* Change of day */
		supply_demand_update();
		break;
	case SIGSWELL: /* Swell */
		wait_time = get_timespec(_data->SO_SWELL_DURATION/24.0);
		do {
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
