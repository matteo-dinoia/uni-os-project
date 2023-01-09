#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/param.h>
#include "header/utils.h"
#include "header/message.h"
#include "header/ipc_manager.h"

/* Global variables */
int _this_id;
list_cargo *cargo_hold;
int extra_supply_created = 0;
int extra_demand_created = 0;

/* Prototypes */
void shop_update();
void signal_handler(int);
void loop();
void respond_msg(int, int, int);
void send_to_ship(int, int, int, int, int);
bool_t receive_from_ship(int *, int *, int *, int *, int *);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;
	sigset_t set_masked;
	int id;

	/* Initialize sigaction */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;

	/* Important signal handler */
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* Get id and data structures */
	_this_id = atoi(argv[1]);
	initialize_ipc_manager(NULL);
	cargo_hold = calloc(SO_MERCI, sizeof(*cargo_hold));

	/* Less important signal handler */
	sigemptyset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGDAY, &sa, NULL);
	sigaction(SIGSWELL, &sa, NULL);

	/* LAST: Start running*/
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	int ship_id, needed_type, needed_amount, n_update, i;

	int last_day_update = 0;
	while (1){
		/* Check if day is changed */
		n_update = get_day() - last_day_update;
		if (n_update > 0){
			remove_port_expired(_this_id, cargo_hold);
			for (i = 0; i < n_update; i++) shop_update();
			last_day_update += n_update;
		}

		/* Respond to ship*/
		if (receive_from_ship(&ship_id, &needed_type, &needed_amount, NULL, NULL) == TRUE){
			respond_msg(ship_id, needed_type, needed_amount);
		}
	}
}

void respond_msg(int ship_id, int needed_type, int needed_amount)
{
	const int this_amount = get_shop_quantity(_this_id, needed_type);

	int tot_exchange = 0, amount = 0, expiry_date = 0;
	int status = STATUS_REFUSED;

	if (needed_amount > 0 && this_amount > 0){
		/* If port is selling respond with how much */
		tot_exchange = MIN(needed_amount, this_amount);

		while (tot_exchange > 0){
			amount = port_sell(_this_id, cargo_hold, tot_exchange, needed_type, &expiry_date);
			if (amount <= 0) {
				send_to_ship(ship_id, needed_type, 0, -1, STATUS_REFUSED);
				break;
			}

			tot_exchange -= amount;
			status = tot_exchange <= 0 ? STATUS_ACCEPTED : STATUS_PARTIAL;
			send_to_ship(ship_id, needed_type, amount, expiry_date, status);
		}
	}else if (needed_amount < 0  && this_amount < 0){
		/* If port is buying respond with how much */
		amount = MIN(-needed_amount, -this_amount);
		port_buy(_this_id, amount, needed_type);
		send_to_ship(ship_id, needed_type, -amount, -1, STATUS_ACCEPTED);
	}else {
		send_to_ship(ship_id, needed_type, 0, -1, STATUS_REFUSED);
	}
}

void send_to_ship(int ship_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, ship_id,
			cargo_type, amount, expiry_date, status);

	send_commerce_msg(get_id_msg_out_ports(), &msg);
}

bool_t receive_from_ship(int *ship_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	return receive_commerce_msg(get_id_msg_in_ports(), _this_id,
			ship_id, cargo_type, amount, expiry_date, status, FALSE);
}

void shop_update()
{
	int rem_supply_tons = get_port_daily_restock_supply(_this_id) - extra_supply_created;
	int rem_demand_tons = get_port_daily_restock_demand(_this_id) - extra_demand_created;
	int rand_type, sum_normalized_first_two, amount;
	bool_t is_demand;
	bool_t is_last_index_used = TRUE;

	/* Do up until there is tons to be generated */
	while (rem_supply_tons > 0 || rem_demand_tons > 0) {
		rand_type = is_last_index_used ? RANDOM(0, SO_MERCI) : (rand_type + 1) % SO_MERCI;

		if (get_shop_quantity(_this_id, rand_type) > 0){
			is_demand = FALSE;
		}else if (get_shop_quantity(_this_id, rand_type) < 0) {
			is_demand = TRUE;
		}else if (rem_supply_tons < rem_demand_tons){
			is_demand = TRUE;
		}else if (rem_supply_tons > rem_demand_tons){
			is_demand = FALSE;
		}else {
			sum_normalized_first_two = GET_SIGN(get_shop_quantity(_this_id, 0))
					+ GET_SIGN(get_shop_quantity(_this_id, 1));
			if (sum_normalized_first_two > 0){
				is_demand = TRUE;
			}else if (sum_normalized_first_two < 0){
				is_demand = FALSE;
			}else is_demand = RANDOM_INCLUDED(0, 1);
		}

		/* Apply the demand or supply if enought tons remains */
		is_last_index_used = TRUE;
		if (is_demand && rem_demand_tons > 0){
			amount = RANDOM_INCLUDED(1, rem_demand_tons / get_cargo_weight_batch(rand_type));
			add_port_demand(_this_id, amount, rand_type);
			rem_demand_tons -= amount * get_cargo_weight_batch(rand_type);
		}
		else if (!is_demand && rem_supply_tons > 0){
			amount = RANDOM_INCLUDED(1, rem_supply_tons / get_cargo_weight_batch(rand_type));
			add_port_supply(_this_id, cargo_hold, amount, rand_type);
			rem_supply_tons -= amount * get_cargo_weight_batch(rand_type);
		}else is_last_index_used = FALSE;
	}

	/* Remember how much i did produced more for compensating */
	extra_demand_created = -rem_demand_tons;
	extra_supply_created = -rem_supply_tons;
}

void signal_handler(int signal)
{
	int i, amount_removed;
	struct timespec rem_time, wait_time;

	switch (signal)
	{
	case SIGDAY: /* Change of day */
		/* Do nothing in the handler for avoiding deadlock on same process only here to stop wait */
		break;
	case SIGSWELL: /* Swell */
		set_port_swell(_this_id);
		wait_event_duration(SO_SWELL_DURATION/24.0, NULL);
		break;
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In port %d (closing)\n", _this_id);
	case SIGINT: /* Normal closing */
		close_all();
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
	close_ipc_manager();

	exit(0);
}
