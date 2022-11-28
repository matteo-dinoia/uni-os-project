#ifndef _BASE_H
#define _BASE_H

#define KEY_SHARED 243  /* TO CHOOSE ONE */

typedef int bool_t; /* Will likely use bool.h */
typedef int id_t;

struct simulation_constant {
	/* Intergers */
	int so_merci, so_fill, so_days, so_navi, so_porti, so_merci;
	int so_size, so_min_vita, so_max_vita, so_lato, so_speed, so_capacity, so_banchine, so_loadspeed;
	int so_storm_duration, so_swell_duration, so_maelestorm;
	id_t shared_bump;
	id_t shared_port;
	id_t shared_ship; /* May not be necessary */

	/* Pointers */
	int *ports_constant;
};

struct port_constant {
	/* Coordinates */
	int x;
	int y;

	int daily_restock_capacity;
};

struct simulation_bumps {
	int n_travelling_cargo;
	int n_travelling_empty;
	int n_processing_cargo;

	int *ship_bumps;
	int *port_bumps;
	int *cargo_bumps;
};

struct ship_bump {
	bool_t affected_by_storm;
};

struct port_bump {
	int n_cargo_received;
	int n_cargo_shipped;
	int n_cargo_present;

	bool_t affected_by_seastorm;
};

/* May not be necessary */
struct cargo_bump {
	int n_cargo_port;
	int n_cargo_ship;
	int n_cargo_delivered;
	int n_cargo_wasted_port;
	int n_cargo_wasted_ship;
};

#endif
