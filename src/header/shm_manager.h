#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include "utils.h"

/* Prototype*/
void initialize_constants();
void close_all();
/* Getter and setter */
double get_constants(int index);
int get_port(int port_id, int var_type);
void set_port(int port_id, int var_type, int var);
int get_ship(int ship_id, int var_type);
void set_ship(int ship_id, int var_type, int var);
int get_cargo(int cargo_id, int var_type);
void set_cargo(int cargo_id, int var_type, int var);
int get_shop(int port_id, int cargo_id, int var_type);
void set_shop(int port_id, int cargo_id, int var_type, int var);
/* Coord */
struct coord get_coord_port(int id);
void set_coord_port(int id, double x, double y);
struct coord get_coord_ship(int id);
void set_coord_ship(int id, double x, double y);
/* Day */
int get_day();
void set_day(int day);


/* Type */
#define VAR_PID 0
/* PORT */
#define VAR_DAILY_RESTOCK 1
#define VAR_DUMP_DOCK_TOT 2
#define VAR_DUMP_HAD_SWELL 3
/* SHIP */
#define VAR_IS_MOVING 4
#define VAR_CAPACITY 5
#define VAR_DUMP_IS_AT_DOCK 6
#define VAR_DUMP_HAD_STORM 7
#define VAR_DUMP_HAD_MAELSTROM 8
/* CARGO */
#define VAR_WEIGHT_BATCH 9
#define VAR_SHELF_LIFE 10
#define VAR_DUMP_AT_PORT 11
#define VAR_DUMP_IN_SHIP 12
#define VAR_DUMP_EXPIRED_PORT 13
#define VAR_DUMP_EXPIRED_SHIP 14
#define VAR_DUMP_TOT_DELIVERED 15
/* SHOP */
#define VAR_QUANTITY 16
#define VAR_DUMP_TOT_SENT 17
#define VAR_DUMP_TOT_RECEIVED 18

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

#endif // SHM_MANAGER_H
