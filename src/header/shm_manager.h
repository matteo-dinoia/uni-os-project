#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include "utils.h"

/* Prototype*/
void initialize_constants();
void start_simulation();
void close_shm_manager();
void close_sem_and_msg();
void print_dump_data();
double get_constants(int index);
/* Setter TODO check permission*/
void increase_day();
void set_ship_dead(int ship_id);
void set_ship_maelstrom(int ship_id);
void set_ship_storm(int ship_id);
void set_ship_pid(int ship_id, pid_t pid);
void set_ship_at_dock(int ship_id, bool_t value, int port_id);
void set_ship_moving(int ship_id, bool_t value);
void set_ship_coord(int ship_id, double x, double y);
int ship_sell(int ship_id, list_cargo *cargo_hold, int amount, int type);
int ship_buy(int ship_id, list_cargo *cargo_hold, int amount, int type, int expiry_date);
void remove_ship_expired(int ship_id, list_cargo *cargo_hold, int increment_day);
void set_port_swell(int port_id);
void set_port_pid(int port_id, pid_t pid);
void port_buy(int port_id, int amount, int type);
int port_sell(int port_id, list_cargo *cargo_hold, int tot_amount, int type, int *expiry_date);
void add_port_demand(int port_id, int amount, int type);
void add_port_supply(int port_id, list_cargo *cargo_hold, int amount, int type);
void remove_port_expired(int port_id, list_cargo *cargo_hold);

/* GETTER */
/* Other */
int get_day();
bool_t check_ships_all_dead();
bool_t check_shop_termination_condition();
id_shared_t get_id_sem_docks();
id_shared_t get_id_msg_in_ports();
id_shared_t get_id_msg_out_ports();
bool_t is_shm_initialized();
/* Port */
struct coord get_port_coord(int id);
int get_port_daily_restock_supply(int port_id);
int get_port_daily_restock_demand(int port_id);
int get_port_pid(int port_id);
int get_port_use(int port_id);
/* Ship */
struct coord get_ship_coord(int id);
bool_t is_ship_dead(int ship_id);
bool_t is_ship_moving(int ship_id);
int get_ship_capacity(int ship_id);
int get_ship_pid(int ship_id);
/* Cargo */
int get_cargo_weight_batch(int cargo_id);
int get_cargo_shelf_life(int cargo_id);
/* Shop */
int get_shop_quantity(int port_id, int cargo_id);

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
