#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

/* Simulation Constants */
#define SO_LATO _SO_LATO
#define SO_DAYS _SO_DAYS
#define SO_NAVI _SO_NAVI
#define SO_PORTI _SO_PORTI
#define SO_MERCI _SO_MERCI
#define SO_STORM_DURATION _SO_STORM_DURATION
#define SO_SWELL_DURATION _SO_SWELL_DURATION
#define SO_MAELSTROM _SO_MAELSTROM
#define SO_FILL _SO_FILL
#define SO_BANCHINE _SO_BANCHINE
#define SO_LOADSPEED _SO_LOADSPEED
#define SO_SIZE _SO_SIZE
#define SO_SPEED _SO_SPEED
#define SO_CAPACITY _SO_CAPACITY
#define SO_MIN_VITA _SO_MIN_VITA
#define SO_MAX_VITA _SO_MAX_VITA

/* Type */
#define PID 0
/* Double*/
#define WHICH_X 0
#define WHICH_Y 1

/* Prototype*/
void initialize_constants();
void close_all();
/* Getter and setter */
int get_ship(int ship_id, int value_type);
void set_ship(int ship_id, int value_type, int value);
int get_port(int port_id, int value_type);
void set_port(int port_id, int value_type, int value);
int get_cargo(int cargo_id, int value_type);
void set_cargo(int cargo_id, int value_type, int value);
int get_supply(int port_id, int cargo_id, int value_type);
void set_supply(int port_id, int cargo_id, int value_type, int value);
/* Coord */
double get_coord_port(int id, int which);
void set_coord_port(int id, double x, double y);
double get_coord_ship(int id, int which);
void set_coord_ship(int id, double x, double y);

#endif // SHM_MANAGER_H
