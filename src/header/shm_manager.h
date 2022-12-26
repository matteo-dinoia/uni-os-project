#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include "utils.h"

/* Simulation Constants */
#define _SO_LATO ((double)get_constants(0))
#define _SO_DAYS ((int)get_constants(1))
#define _SO_NAVI ((int)get_constants(2))
#define _SO_PORTI ((int)get_constants(3))
#define _SO_MERCI ((int)get_constants(4))
#define _SO_STORM_DURATION ((int)get_constants(5))
#define _SO_SWELL_DURATION ((int)get_constants(6))
#define _SO_MAELSTROM ((int)get_constants(7))
#define _SO_FILL ((int)get_constants(8))
#define _SO_BANCHINE ((int)get_constants(9))
#define _SO_LOADSPEED ((int)get_constants(10))
#define _SO_SIZE ((int)get_constants(11))
#define _SO_SPEED ((int)get_constants(12))
#define _SO_CAPACITY ((int)get_constants(13))
#define _SO_MIN_VITA ((int)get_constants(14))
#define _SO_MAX_VITA ((int)get_constants(15))

/* Type */
#define PID 0
/* Double*/
#define WHICH_X 0
#define WHICH_Y 1

/* Prototype*/
void initialize_constants();
void close_all();
/* Getter and setter */
double get_constants(int index);
int get_ship(int ship_id, int value_type);
void set_ship(int ship_id, int value_type, int value);
int get_port(int port_id, int value_type);
void set_port(int port_id, int value_type, int value);
int get_cargo(int cargo_id, int value_type);
void set_cargo(int cargo_id, int value_type, int value);
int get_supply(int port_id, int cargo_id, int value_type);
void set_supply(int port_id, int cargo_id, int value_type, int value);
/* Coord */
struct coord get_coord_port(int id);
void set_coord_port(int id, double x, double y);
struct coord get_coord_ship(int id);
void set_coord_ship(int id, double x, double y);
/* Day */
int get_day();
void set_day(int day);

#endif // SHM_MANAGER_H
