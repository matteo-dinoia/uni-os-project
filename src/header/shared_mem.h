#ifndef _SHARED_MEMORY_H
#define _SHARED_MEMORY_H

#include <sys/types.h>
#include <sys/shm.h>
#include "utils.h"
#define KEY_SHARED 0xf3

/* Prototype */
void detach(void *pointer);
id_shared_t get_shared(key_t key, size_t size);
void *attach_shared(id_shared_t id, int extra_flags);

struct general{
	/* Constants*/
	double so_lato; /* DO NOT TOUCH ORDER */
	int so_days, so_navi, so_porti, so_merci;	/* Generic simulation specifications */
	int so_storm_duration, so_swell_duration, so_maelstrom; /* Weather events max duration */
	int so_fill, so_banchine, so_loadspeed;			/* Ports specifications */
	int so_size, so_speed, so_capacity;			/* Ships specifications */
	int so_min_vita, so_max_vita;				/* Cargo specifications */

	/* Today */
	int today;
};

struct port{ /* Writers: port */
	struct coord coordinates;
	pid_t pid;
	int daily_restock_capacity;

	/* Dump */
	int dump_dock_tot; /* const value */
	int dump_had_swell; /* DONE */
};

struct ship{ /* Writers: ship */
	struct coord coordinates;
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

struct shop{ /* Writers: port */
	int quantity;

	/* Dump */
	int dump_tot_sent; /* DONE */
	int dump_tot_received; /* DONE */
};

#endif
