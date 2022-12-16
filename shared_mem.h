#ifndef _SHARED_MEMORY_H
#define _SHARED_MEMORY_H

#define KEY_SHARED 0xf3 /* TO CHOOSE ONE */
#define TRUE 1
#define FALSE 0
typedef int bool_t; /* Will likely use bool.h */
typedef int id_shm_t;

/* Prototype */
void detach(void *);
id_shm_t get_shared(key_t, size_t);
void *attach_shared(id_shm_t);

struct const_general{
	/* Constants*/
	int SO_LATO, SO_DAYS, SO_NAVI, SO_PORTI, SO_MERCI;	/* Generic simulation specifications */
	int SO_STORM_DURATION, SO_SWELL_DURATION, SO_MAELSTORM; /* Weather events max duration */
	int SO_FILL, SO_BANCHINE, SO_LOADSPEED;			/* Ports specifications */
	int SO_SIZE, SO_SPEED, SO_CAPACITY;			/* Ships specifications */
	int SO_MIN_VITA, SO_MAX_VITA;				/* Cargo specifications */

	/* Shared memory id */
	id_shm_t id_const_port;
	id_shm_t id_const_ship;
	id_shm_t id_const_cargo;

	/* Msg id */
	id_shm_t id_msg_bump; /* still not in use */
	id_shm_t id_msg_in_ports;
	id_shm_t id_msg_out_ports;

	/* Semaphores */
	id_shm_t id_sem_docks;
};

struct const_port{
	/* Coordinates */
	double x;
	double y;

	pid_t pid;
	int daily_restock_capacity;
};

struct const_ship{
	/* Coordinates */
	double x;
	double y;

	pid_t pid;
	bool_t is_moving;
};

struct const_cargo{
	/* Coordinates */
	int weight_batch;
	int shelf_life;
};

#endif
