#ifndef _SHARED_MEMORY_H
#define _SHARED_MEMORY_H

#include <sys/types.h>
#include <sys/shm.h>
#include "utils.h"

/* Shared memory keys */
#define KEY_SHM_GENERAL 0x10ff
#define KEY_SHM_PORT 0x20ff
#define KEY_SHM_SHIP 0x30ff
#define KEY_SHM_CARGO 0x40ff
#define KEY_SHM_SHOP 0x50ff

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
	int daily_restock_capacity;
	pid_t pid;

	/* Dump */
	int dump_dock_tot; /* const value */
	int dump_had_swell; /* DONE */
};

struct ship{ /* Writers: ship */
	struct coord coordinates;
	bool_t is_moving;
	bool_t is_dead;
	pid_t pid;

	/* Dump */
	int capacity; /* DONE -> if equals to SO_CAPACITY is empty*/
	bool_t dump_is_at_dock; /* DONE */
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
	int dump_delivered_unwanted;
};

struct shop{ /* Writers: port */
	int quantity;

	/* Dump */
	int dump_tot_sent; /* DONE */
	int dump_tot_received; /* DONE */
};

#endif
