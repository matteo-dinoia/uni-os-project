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
	int SO_STORM_DURATION, SO_SWELL_DURATION, SO_MAELSTROM; /* Weather events max duration */
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
	id_shared_t id_sem_cargo;

	/* Today */
	int today;
};

struct port{ /* Writers: port */
	double x;
	double y;
	pid_t pid;
	int daily_restock_capacity;

	/* Dump */
	int dump_dock_tot; /* const value */
	int dump_had_swell; /* DONE */
};

struct ship{ /* Writers: ship */
	double x;
	double y;
	pid_t pid;
	bool_t is_moving;

	/* Dump */
	bool_t dump_is_at_dock; /* DONE */
	int capacity; /* DONE -> if equals to SO_CAPACITY is empty*/
	int dump_had_storm; /* DONE */
	int dump_had_maelstrom; /* DONE */
};

struct cargo{ /* Writers: port, ship */
	int weight_batch;
	int shelf_life;

	/* Dump */
	int dump_at_port; /* DONE */
	int dump_in_ship; /* DONE */
	int dump_exipered_port; /* DONE */
	int dump_exipered_ship;
	int dump_tot_delivered; /* DONE */
};

struct supply_demand{ /* Writers: port */
	int quantity;

	/* Dump */
	int dump_tot_sent; /* DONE */
	int dump_tot_received; /* DONE */
};

#endif
