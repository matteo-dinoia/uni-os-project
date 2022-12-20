#ifndef _SHARED_MEMORY_H
#define _SHARED_MEMORY_H

#include "utils.h"
#define KEY_SHARED 0xf3 /* TODO choose one */

/* Prototype */
void detach(void *pointer);
id_shared_t get_shared(key_t key, size_t size);
void *attach_shared(id_shared_t id);

struct general{
	/* Constants*/
	int SO_LATO, SO_DAYS, SO_NAVI, SO_PORTI, SO_MERCI;	/* Generic simulation specifications */
	int SO_STORM_DURATION, SO_SWELL_DURATION, SO_MAELSTORM; /* Weather events max duration */
	int SO_FILL, SO_BANCHINE, SO_LOADSPEED;			/* Ports specifications */
	int SO_SIZE, SO_SPEED, SO_CAPACITY;			/* Ships specifications */
	int SO_MIN_VITA, SO_MAX_VITA;				/* Cargo specifications */

	/* Shared memory id */
	id_shared_t id_port;
	id_shared_t id_ship;
	id_shared_t id_cargo;
	id_shared_t id_supply_demand;

	/* Msg id */
	id_shared_t id_msg_in_ports;
	id_shared_t id_msg_out_ports;

	/* Semaphores */
	id_shared_t id_sem_docks;
};

struct port{
	double x;
	double y;
	pid_t pid;
	int daily_restock_capacity;

	/* Dump */
	int dump_dock_used;
	int dump_dock_tot;
};

struct ship{
	double x;
	double y;
	pid_t pid;
	bool_t is_moving;

	/* Dump */
	bool_t dump_is_empty;
	bool_t dump_is_at_port;
	bool_t dump_is_at_dock;
};

struct cargo{
	int weight_batch;
	int shelf_life;

	/* Dump */
	int dump_at_port;
	int dump_in_ship;
	int dump_tot_delivered;
	int dump_exipered_ship;
	int dump_exipered_port;
};

struct supply_demand{
	int quantity;

	/* Dump */
	int dump_tot_sent;
	int dump_tot_received;
};

#endif
