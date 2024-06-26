#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <math.h>
#include "header/message.h"
#include "header/ipc_manager.h"

/* Macro*/
#define GET_TRAVEL_TIME(this_coord, dest_coord, speed)\
	(sqrt(pow((this_coord.x) - (dest_coord.x), 2) + pow((this_coord.y) - (dest_coord.y), 2)) / (double) (speed))

/* Global Variables */
int _this_id;
int _next_port_destination = -1;
list_cargo *cargo_hold;
int last_day_update = 0; /* For expired cargo */

/* Prototypes */
void get_next_destination_port(int *, struct coord *);
int new_destiation_port(int);
void move_to_port(struct coord);
void exchange_cargo(int);
void signal_handler(int);
void loop();
int sell(int, int);
int buy(int, int, int);
int pick_buy(int, int, int);
void send_to_port(int, int, int, int, int);
void receive_from_port(int *, int *, int *, int *, int *);
void check_if_cargo_expired();
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	int id;
	struct sigaction sa;
	sigset_t set_masked;

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
	sigaction(SIGSTORM, &sa, NULL);
	sigaction(SIGMAELSTROM, &sa, NULL);
	sigaction(SIGDAY, &sa, NULL);

	/* LAST: Start running*/
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	int dest_port;
	struct coord dest_coord;

	while (1){
		get_next_destination_port(&dest_port, &dest_coord);
		move_to_port(dest_coord);
		exchange_cargo(dest_port);
	}
}

void get_next_destination_port(int *dest, struct coord *dest_coord)
{
	if (_next_port_destination < 0) /* No destination set still */
		new_destiation_port(-1);

	/* Set port */
	*dest = _next_port_destination;
	_next_port_destination = -1;
	/* get position */
	*dest_coord = get_port_coord(*dest);
}

int new_destiation_port(int current_port)
{
	int port, cargo_type, sale, not_expired, request_port, min;
	int best_port = -1, best_sale;
	int worst_port = -1, worst_use, use, start_port, i, noise, max_noise;
	double best_travel_time, travel_time;

	/* If empty */
	if (get_ship_capacity(_this_id) >= SO_CAPACITY){
		max_noise = ROUNDUP(SO_NAVI / 50);
		start_port = RANDOM(0, SO_PORTI);
		for (i = 0; i < SO_PORTI; i++){
			port = (i + start_port) % SO_PORTI;

			/* Skip current port */
			if (port == current_port) continue;

			/* Calculate use and choose worst */
			noise = RANDOM_INCLUDED(-max_noise, max_noise);
			use = get_port_use(port) + noise;
			if (worst_port == -1 /* still doesn't have a worst port*/
					|| use < worst_use){ /* or is worst */
				worst_port = port;
				worst_use = use;
			}
		}
		return (_next_port_destination = worst_port);
	}

	/* If has already cargo */
	for (port = 0; port < SO_PORTI; port++){
		/* Skip current port */
		if (port == current_port) continue;

		/* Calculate Travel time */
		travel_time = GET_TRAVEL_TIME(get_ship_coord(_this_id), get_port_coord(port), SO_SPEED);

		/* Calculate Sale */
		sale = 0;
		for (cargo_type = 0; cargo_type < SO_MERCI; cargo_type++){
			not_expired = get_not_expired_by_day(&cargo_hold[cargo_type],
					get_day() + (int) travel_time);
			request_port = -get_shop_quantity(port, cargo_type);

			min = MIN(not_expired, request_port);
			sale += min > 0 ? min : 0;
		}

		/* Check if is new best */
		if (best_port == -1 /* still doesn't have a best port */
				|| sale > best_sale /* or is best */
				|| (best_sale == sale && travel_time < best_travel_time)){
			best_port = port;
			best_sale = sale;
			best_travel_time = travel_time;
		}
	}

	return (_next_port_destination = best_port);
}

void move_to_port(struct coord dest_coord)
{
	double time = GET_TRAVEL_TIME(get_ship_coord(_this_id), dest_coord, SO_SPEED);

	/* Wait */
	set_ship_moving(_this_id, TRUE);
	wait_event_duration(time, &check_if_cargo_expired);
	set_ship_moving(_this_id, FALSE);

	/* Actual move*/
	set_ship_coord(_this_id, dest_coord.x, dest_coord.y);
}

void check_if_cargo_expired()
{
	const int today = get_day();
	if(last_day_update >= today) return;

	remove_ship_expired(_this_id, cargo_hold, 0);
	last_day_update = today;

}

void exchange_cargo(int port_id)
{
	sigset_t set_masked;
	int tons_moved, i, type, amount, dest_port_id, start_type, days_needed;

	/* Get dock */
	execute_single_sem_oper(get_id_sem_docks(), port_id, -1);
	set_ship_at_dock(_this_id, TRUE, port_id);

	/* Initialize signal ignored */
	sigemptyset(&set_masked);
	sigaddset(&set_masked, SIGMAELSTROM);

	/* Selling */
	for (type = 0; type < SO_MERCI; type++){
		check_if_cargo_expired();

		/* Actual sell (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = sell(port_id, type);
		sigprocmask(SIG_UNBLOCK, &set_masked, NULL);

		/* Wait */
		wait_event_duration(tons_moved / (double)SO_LOADSPEED, NULL);
	}

	/* New Dest and discard cargo that cannot make the travel */
	dest_port_id = new_destiation_port(port_id);
	days_needed = GET_TRAVEL_TIME(get_ship_coord(_this_id), get_port_coord(dest_port_id), SO_SPEED);
	if (days_needed > 0) remove_ship_expired(_this_id, cargo_hold, days_needed);

	/* Buying */
	start_type = RANDOM(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		check_if_cargo_expired();

		/* Pick how much to buy */
		type = (type + i) % SO_MERCI;
		amount = pick_buy(port_id, dest_port_id, type);
		if(amount <= 0) continue; /* Cannot buy anything */

		/* Actual buy (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = buy(port_id, type, amount);
		sigprocmask(SIG_UNBLOCK, &set_masked, NULL);

		/* Wait */
		wait_event_duration(tons_moved / (double)SO_LOADSPEED, NULL);
	}

	/* Free dock */
	execute_single_sem_oper(get_id_sem_docks(), port_id, 1);
	set_ship_at_dock(_this_id, FALSE, port_id);
}

int sell(int port_id, int type_to_sell)
{
	int amount, amount_port, amount_ship;
	int weight, status;

	/* Min i have cargo and ports need it */
	amount_port = -get_shop_quantity(port_id, type_to_sell);
	amount_ship = count_cargo(&cargo_hold[type_to_sell]);
	amount = MIN(amount_ship, amount_port);
	/* If nothing to be sell then skip this type*/
	if (amount <= 0) return 0;

	/* Send and receive message */
	send_to_port(port_id, type_to_sell, -amount, -1, STATUS_REQUEST);
	receive_from_port(NULL, NULL, &amount, NULL, &status);

	/* Change data */
	if (status == STATUS_ACCEPTED && amount < 0){
		/* Return the WEIGHT of quantity sold */
		return ship_sell(_this_id, cargo_hold, amount, type_to_sell);
	}

	return 0;
}

int buy(int port_id, int type_to_buy, int amount_to_buy)
{
	int amount, weight, tons_moved, type, expiry_date, status;

	/* Send request*/
	send_to_port(port_id, type_to_buy, amount_to_buy, -1, STATUS_REQUEST);

	/* Wait response */
	tons_moved = 0;
	do {
		receive_from_port(NULL, &type, &amount, &expiry_date, &status);
		/* Change data */
		if (status == STATUS_PARTIAL || status == STATUS_ACCEPTED){
			/* Return the WEIGHT of quantity buougt */
			tons_moved += ship_buy(_this_id, cargo_hold, amount, type, expiry_date);
		}
	}while(status == STATUS_PARTIAL);

	return tons_moved;
}

int pick_buy(int port_id, int dest_port_id, int type)
{
	int n_sell_this_port, n_capacity, n_buy_dest_port, res;

	n_sell_this_port = get_shop_quantity(port_id, type);
	n_buy_dest_port = -get_shop_quantity(dest_port_id, type);
	if (n_buy_dest_port < 0) n_buy_dest_port = 0;
	n_capacity = get_ship_capacity(_this_id) / get_cargo_weight_batch(type);

	/* Calculate min value */
	return MIN(MIN(n_sell_this_port, n_buy_dest_port + RANDOM(0, ROUNDUP(n_capacity/4))), n_capacity);
}

void send_to_port(int port_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, port_id,
			cargo_type, amount, expiry_date, status);

	send_commerce_msg(get_id_msg_in_ports(), &msg);
}

void receive_from_port(int *port_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	receive_commerce_msg(get_id_msg_out_ports(), _this_id,
			port_id, cargo_type, amount, expiry_date, status, TRUE);
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;
	int i, amount_removed;

	switch (signal){
	case SIGDAY:
		/* Do nothing in the handler for avoiding data incoherence only here to stop wait */
		break;
	case SIGSTORM: /* Storm */
		set_ship_storm(_this_id);
		wait_event_duration(SO_STORM_DURATION/24.0, NULL);
		break;
	case SIGMAELSTROM: /* Maeltrom */
		set_ship_maelstrom(_this_id);
		remove_ship_expired(_this_id, cargo_hold, 0);
		close_all();
		break;
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In ship pid: %d (closing)\n", _this_id);
	case SIGINT: /* Normal closing */
		close_all();
		break;
	}
}

void close_all()
{
	int i, amount;

	/* Signaling data of death */
	if (is_ipc_initialized())set_ship_dead(_this_id);

	/* Local memory deallocation */
	for (i = 0; i < SO_MERCI; i++)
		free_cargo(&cargo_hold[i]);
	free(cargo_hold);

	/* Detach shared memory */
	close_ipc_manager();

	exit(0);
}
