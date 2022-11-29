#ifndef _BASE_H
#define _BASE_H

#define KEY_SHARED 243 /* TO CHOOSE ONE */

typedef int bool_t; /* Will likely use bool.h */
typedef int id_t;

/* Prototype */
struct sembuf create_sembuf(int, int);

/* CONST */

struct const_general
{
	/* Constants*/
	int SO_LATO, SO_DAYS, SO_NAVI, SO_PORTI, SO_MERCI;	/* Generic simulation specifications */
	int SO_STORM_DURATION, SO_SWELL_DURATION, SO_MAELSTORM; /* Weather events max duration */
	int SO_FILL, SO_BANCHINE, SO_LOADSPEED;			/* Ports specifications */
	int SO_SIZE, SO_SPEED, SO_CAPACITY;			/* Ships specifications */
	int SO_MIN_VITA, SO_MAX_VITA;				/* Cargo specifications */

	/* Shared memory id */
	id_t id_const_port;
	id_t id_const_ship;
	id_t id_bump_general;
	id_t id_bump_port;
	id_t id_bump_ship; /* Maybe not necessary */
	id_t id_bump_cargo;
};

struct const_port
{
	/* Coordinates */
	int x;
	int y;

	int daily_restock_capacity;
};

struct const_ship
{
	/* Coordinates */
	int x;
	int y;

	bool_t is_moving;
};

/* BUMP */

struct bump_general
{
	int n_travelling_cargo;
	int n_travelling_empty;
	int n_processing_cargo;

	int *ship_bumps;
	int *port_bumps;
	int *cargo_bumps;
};

struct bump_ship
{
	bool_t affected_by_storm;
};

struct bump_port
{
	int n_cargo_received;
	int n_cargo_shipped;
	int n_cargo_present;

	bool_t affected_by_seastorm;
};

struct bump_cargo /* May not be necessary */
{
	int n_cargo_port;
	int n_cargo_ship;
	int n_cargo_delivered;
	int n_cargo_wasted_port;
	int n_cargo_wasted_ship;
};

#endif
