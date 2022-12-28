#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include "utils.h"

/* Prototype*/
void initialize_constants();
void close_all();
double get_constants(int index);
/* Setter */
void set_coord_port(int id, double x, double y);
void set_coord_ship(int id, double x, double y);
void set_day(int day);

/* GETTER */
/* Day */
int get_day();
/* Port */
struct coord get_coord_port(int id);
int get_port_daily_restock(int port_id);
int get_port_tot_dock(int port_id);
bool_t had_port_swell(int port_id);
/* Ship */
struct coord get_coord_ship(int id);
bool_t is_ship_moving(int ship_id);
int get_ship_capacity(int ship_id);
bool_t is_ship_at_port(int ship_id);
bool_t had_ship_storm(int ship_id);
bool_t had_ship_maelstrom(int ship_id);
/* Cargo */
int get_cargo_weight_batch(int cargo_id);
int get_cargo_shelf_life(int cargo_id);
int get_cargo_at_port(int cargo_id);
int get_cargo_in_ship(int cargo_id);
int get_cargo_expired_port(int cargo_id);
int get_cargo_expired_ship(int cargo_id);
int get_cargo_tot_delivered(int cargo_id);
/* Shop */
int get_shop_quantity(int port_id, int cargo_id);
int get_shop_tot_sent(int port_id, int cargo_id);
int get_shop_tot_received(int port_id, int cargo_id);

/* Simulation Constants */
#define SO_LATO ((double)get_constants(0))
#define SO_DAYS ((int)get_constants(1))
#define SO_NAVI ((int)get_constants(2))
#define SO_PORTI ((int)get_constants(3))
#define SO_MERCI ((int)get_constants(4))
#define SO_STORM_DURATION ((int)get_constants(5))
#define SO_SWELL_DURATION ((int)get_constants(6))
#define SO_MAELSTROM ((int)get_constants(7))
#define SO_FILL ((int)get_constants(8))
#define SO_BANCHINE ((int)get_constants(9))
#define SO_LOADSPEED ((int)get_constants(10))
#define SO_SIZE ((int)get_constants(11))
#define SO_SPEED ((int)get_constants(12))
#define SO_CAPACITY ((int)get_constants(13))
#define SO_MIN_VITA ((int)get_constants(14))
#define SO_MAX_VITA ((int)get_constants(15))

/* Permissions in binary */
#define PORT_WRITE 1 /* 0001 */
#define SHIP_WRITE 2 /* 0010 */
#define CARGO_WRITE 4 /* 0100 */
#define SHOP_WRITE 8 /* 1000 */

#endif
