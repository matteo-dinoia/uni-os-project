#define _GNU_SOURCE
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/param.h>
#include "header/shared_mem.h"
#include "header/message.h"
#include "header/semaphore.h"
#include "header/utils.h"
#include "header/shm_manager.h"

/* Macro*/
#define GET_TRAVEL_TIME(this_coord, dest_coord, speed)\
	(sqrt(pow((this_coord.x) - (dest_coord.x), 2) + pow((this_coord.y) - (dest_coord.y), 2)) / (double) (speed))

/* Global Variables */
int _this_id;
int _next_port_destination;
list_cargo *cargo_hold;

/* Prototypes */
void get_next_destination_port(int *, struct coord *);
int new_destiation_port(int);
void discard_expiring_cargo(int);
void move_to_port(struct coord);
void exchange_cargo(int);
void signal_handler(int);
void loop();
int sell(int, int);
int buy(int, int, int);
int pick_buy(int, int, int);
void send_to_port(int, int, int, int, int);
void receive_from_port(int *, int *, int *, int *, int *);
void close_all();


int main(int argc, char *argv[])
{
	/* Variables */
	int id;
	struct sigaction sa;
	sigset_t set_masked;

	/* Get id */
	_this_id = atoi(argv[1]);

	/* Get data structures */
	initialize_shm_manager(SHIP_WRITE | CARGO_WRITE, NULL);
	cargo_hold = calloc(SO_MERCI, sizeof(*cargo_hold));

	/* LAST: Setting signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;
	sigaction(SIGSTORM, &sa, NULL);
	sigaction(SIGMAELSTROM, &sa, NULL);
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
	int dest_port;
	struct coord dest_coord;

	while (1){
		get_next_destination_port(&dest_port, &dest_coord);
		move_to_port(dest_coord);
		dprintf(1, "\t[Ship %d] arrived at %d (x: %lf, y: %lf)\n", _this_id, dest_port, dest_coord.x, dest_coord.y);
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
	int worst_port = -1, worst_use, use, start_port, i;
	double best_travel_time, travel_time;

	/* If empty */
	if (get_ship_capacity(_this_id) == SO_CAPACITY){
		start_port = RANDOM(0, SO_PORTI);
		for (i = 0; i < SO_PORTI; i++){
			port = (i + start_port) % SO_PORTI;

			/* Skip current port */
			if (port == current_port) continue;

			/* Calculate use and choose worst */
			use = get_port_use(port);
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

void discard_expiring_cargo(int dest_id)
{
	/* Time */
	int days_needed = GET_TRAVEL_TIME(get_ship_coord(_this_id), get_port_coord(dest_id), SO_SPEED);

	/* Remove cargo */
	remove_ship_expired(_this_id, cargo_hold, days_needed);
}

void move_to_port(struct coord dest_coord)
{
	double time = GET_TRAVEL_TIME(get_ship_coord(_this_id), dest_coord, SO_SPEED);

	/* Wait */
	set_ship_moving(_this_id, TRUE);
	wait_event_duration(time);
	set_ship_moving(_this_id, FALSE);

	/* Actual move*/
	set_ship_coord(_this_id, dest_coord.x, dest_coord.y);
}

void exchange_cargo(int port_id)
{
	struct sembuf sem_buf;
	sigset_t set_masked;
	int tons_moved, i, type, amount, dest_port_id, start_type;

	/* Get dock */
	execute_single_sem_oper(get_id_sem_docks(), port_id, -1);
	set_ship_at_dock(_this_id, TRUE);

	/* Initialize signal ignored TODO move inside*/
	sigemptyset(&set_masked);
	sigaddset(&set_masked, SIGMAELSTROM);

	/* Selling */
	for (type = 0; type < SO_MERCI; type++){
		/* Actual sell (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = sell(port_id, type);
		sigprocmask(SIG_UNBLOCK, &set_masked, NULL);

		/* Wait */
		wait_event_duration(tons_moved / (double)SO_LOADSPEED);
	}

	/* New Dest */
	dest_port_id = new_destiation_port(port_id);
	discard_expiring_cargo(dest_port_id);

	/* Buying */
	start_type = RANDOM(0, SO_MERCI);
	for (i = 0; i < SO_MERCI; i++){
		/* Pick how much to buy */
		type = (type + i) % SO_MERCI;
		amount = pick_buy(port_id, dest_port_id, type);
		if(amount <= 0) continue; /* Cannot buy anything */

		/* Actual buy (unsinkable) */
		sigprocmask(SIG_BLOCK, &set_masked, NULL);
		tons_moved = buy(port_id, type, amount);
		sigprocmask(SIG_UNBLOCK, &set_masked, NULL);

		/* Wait */
		wait_event_duration(tons_moved / (double)SO_LOADSPEED);
	}

	/* Free dock */
	execute_single_sem_oper(get_id_sem_docks(), port_id, 1);
	set_ship_at_dock(_this_id, FALSE);
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
	int n_sell_this_port, n_capacity, n_buy_dest_port;

	n_sell_this_port = get_shop_quantity(port_id, type);
	n_buy_dest_port = -get_shop_quantity(dest_port_id, type);
	n_capacity = get_ship_capacity(_this_id) / get_cargo_weight_batch(type);

	/* Calculate min value */
	return MIN(MIN(n_sell_this_port, n_buy_dest_port + RANDOM(0, ROUNDUP(n_capacity/4))), n_capacity);
}

void send_to_port(int port_id, int cargo_type, int amount, int expiry_date, int status)
{
	struct commerce_msgbuf msg;
	create_commerce_msgbuf(&msg, _this_id, port_id,
			cargo_type, amount, expiry_date, status);

	dprintf(1, "SHIP %d SEND TO PORT %d REQUEST amount %d cargo_type %d\n", _this_id, port_id, amount, cargo_type);
	send_commerce_msg(get_id_msg_in_ports(), &msg);
}

void receive_from_port(int *port_id, int *cargo_type, int *amount, int *expiry_date, int *status)
{
	/* dprintf(1, "SHIP %d LISTEN TO PORTS\n", _this_id); */
	receive_commerce_msg(get_id_msg_out_ports(), _this_id,
			port_id, cargo_type, amount, expiry_date, status);
	dprintf(1, "SHIP %d RECEIVED FROM PORTS status %d amount %d\n", _this_id, *status, *amount);
}

void signal_handler(int signal)
{
	struct timespec rem_time, wait_time;
	int i, amount_removed;

	switch (signal){
	case SIGDAY:
		remove_ship_expired(_this_id, cargo_hold, 0);
		break;
	case SIGSEGV:
		dprintf(1, "[SEGMENTATION FAULT] In ship (closing)");
		close_all();
		break;
	case SIGMAELSTROM: /* Maeltrom -> sinks the ship */
		set_ship_maelstrom(_this_id);
		remove_ship_expired(_this_id, cargo_hold, 0);
	case SIGINT: /* Closing for every other reason */
		close_all();
		break;
	case SIGSTORM: /* Storm -> stops the ship for STORM_DURATION time */
		set_ship_storm(_this_id);
		wait_event_duration(SO_STORM_DURATION/24.0);
		break;
	}
}

void close_all()
{
	int i, amount;

	/* Signaling data of death */
	set_ship_dead(_this_id);

	/* Local memory deallocation */
	for (i = 0; i < SO_MERCI; i++)
		free_cargo(&cargo_hold[i]);
	free(cargo_hold);

	/* Detach shared memory */
	close_shm_manager();

	exit(0);
}
