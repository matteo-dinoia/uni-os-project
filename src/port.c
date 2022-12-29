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
#include "header/shm_manager.h"

/* Global variables */
int _this_id;
list_cargo *cargo_hold;

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

	/* Get id */
	_this_id = atoi(argv[1]);

	/* Initialize structures */
	initialize_shm_manager(PORT_WRITE | CARGO_WRITE | SHOP_WRITE, NULL);
	cargo_hold = calloc(SO_MERCI, sizeof(*cargo_hold));

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
			_this_supply_demand[needed_type].quantity -= amount;
			status = tot_exchange <= 0 ? STATUS_ACCEPTED : STATUS_PARTIAL;
			send_to_ship(ship_id, needed_type, amount, expiry_date, status);

			/* Dump */
			execute_single_sem_oper(_data->id_sem_cargo, needed_type, -1);
			_data_cargo[needed_type].dump_at_port -= abs(amount);
			_data_cargo[needed_type].dump_in_ship += abs(amount);
			_this_supply_demand[needed_type].dump_tot_sent += abs(amount);
			execute_single_sem_oper(_data->id_sem_cargo, needed_type, 1);
		}
		if(tot_exchange <= 0)
			return;
	}else if (needed_amount < 0  && this_amount < 0){
		/* If port is buying respond with how much */
		amount = -MIN(-needed_amount, -this_amount);
		_this_supply_demand[needed_type].quantity += abs(amount);
		status = STATUS_ACCEPTED;

		/* Dump */
		execute_single_sem_oper(_data->id_sem_cargo, needed_type, -1);
		_data_cargo[needed_type].dump_at_port += abs(amount);
		_data_cargo[needed_type].dump_in_ship -= abs(amount);
		_data_cargo[needed_type].dump_tot_delivered += abs(amount);
		_this_supply_demand[needed_type].dump_tot_received += abs(amount);
		execute_single_sem_oper(_data->id_sem_cargo, needed_type, 1);
	}

	/* Refuse or send message */
	send_to_ship(ship_id, needed_type, amount, expiry_date, status);
}

void send_to_ship(int ship_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, ship_id,
			cargo_type, amount, expiry_date, status);

	/* dprintf(1, "PORT %d SEND TO SHIP %d\n", _this_id, ship_id); */
	send_commerce_msg(_data->id_msg_out_ports, &msg);
}

void receive_from_ship(int *ship_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	/* dprintf(1, "PORT %d LISTEN TO SHIPS\n", _this_id); */
	receive_commerce_msg(_data->id_msg_in_ports, _this_id,
			ship_id, cargo_type, amount, expiry_date, status);
	/* dprintf(1, "PORT %d RECEIVED FROM SHIPS\n", _this_id); */
}

void supply_demand_update()
{
	int rem_offer_tons = _this_port->daily_restock_capacity;
	int rem_demand_tons = _this_port->daily_restock_capacity;
	int rand_type, sum_normalized_first_two;
	bool_t is_demand;

	/* TODO avoid going over the limits */
	while (rem_offer_tons > 0 || rem_demand_tons > 0) {
		rand_type = RANDOM(0, SO_MERCI);
		if (_this_supply_demand[rand_type].quantity > 0){
			is_demand = FALSE;
		}else if (_this_supply_demand[rand_type].quantity < 0) {
			is_demand = TRUE;
		}else if (rem_offer_tons < rem_demand_tons){
			is_demand = TRUE;
		}else if (rem_offer_tons > rem_demand_tons){
			is_demand = FALSE;
		}else {
			sum_normalized_first_two = GET_SIGN(_this_supply_demand[0].quantity)
					+ GET_SIGN(_this_supply_demand[1].quantity);
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

			/* Dump */
			execute_single_sem_oper(_data->id_sem_cargo, rand_type, -1);
			_data_cargo[rand_type].dump_at_port += 1;
			execute_single_sem_oper(_data->id_sem_cargo, rand_type, 1);
		}
	}
}

void signal_handler(int signal)
{
	int i, amount_removed;
	struct timespec rem_time, wait_time;

	switch (signal)
	{
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In port (closing)");
	case SIGINT:
		close_all();
		break;
	case SIGDAY: /* Change of day */
		for (i = 0; i < SO_MERCI; i++){
			amount_removed = remove_expired_cargo(&cargo_hold[i], _data->today);
			_this_supply_demand[i].quantity -= amount_removed;

			/* Bump */
			execute_single_sem_oper(_data->id_sem_cargo, i, -1);
			_data_cargo[i].dump_at_port -= amount_removed;
			_data_cargo[i].dump_exipered_port += amount_removed;
			execute_single_sem_oper(_data->id_sem_cargo, i, 1);
		}
		supply_demand_update();
		break;
	case SIGSWELL: /* Swell */
		set_port_swell(_this_id);
		wait_event_duration(SO_SWELL_DURATION/24.0);
		break;
	}
}

void close_all()
{
	int i;

	/* Local memory deallocation */
	for (i = 0; i < SO_MERCI; i++)
		free_cargo(&cargo_hold[i]);
	free(cargo_hold);

	/* Detach shared memory */
	close_shm_manager();

	exit(0);
}
