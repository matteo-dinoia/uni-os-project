#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

/* Simulation Constants */
#define SO_DAYS (_SO_DAYS)

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