#define _GNU_SOURCE

/* Libraries */
#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "shared_mem.h"
#include "semaphore.h"

/* Macros */
#define SHIP_EXEC "./ship"
#define PORT_EXEC "./port"

/* Global variables */

/* Shared memory */
struct const_general *_data;
struct const_port *_data_port;
struct const_ship *_data_ship;
struct const_cargo *_data_cargo;
int *_data_supply_demand;

int id_data;
int _id_sem;

/* Prototypes */
void initialize_shared();
pid_t create_proc(char *, int);
void custom_handler(int);
void close_all(const char *, int);
void create_children();
void read_constants_from_file();
void create_shared_structures();
double get_random_coord();
int get_random(int, int);
void loop();

int main()
{
	struct sigaction sa;
	sigset_t set_masked;

	/* Initializing */
	srand(time(NULL));
	initialize_shared();

	/* Create and start children*/
	create_children();
	execute_single_sem_oper(_id_sem, 0, -1);

	/* Setting signal handler (need to be done after) */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &custom_handler;
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* Start running*/
	loop();
}

void initialize_shared()
{
	int id;

	/* SHM: General */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600 | IPC_CREAT | IPC_EXCL);
	id_data = id;
	_data = shmat(id, NULL, 0);
	read_constants_from_file();

	/* SHM: Port */
	id = shmget(IPC_PRIVATE, sizeof(*_data_port) * _data->SO_PORTI, 0600);
	_data_port = shmat(id, NULL, 0);
	_data->id_const_port = id;

	/* SHM: Suppy and demand of ports*/
	id = shmget(IPC_PRIVATE, sizeof(*_data_supply_demand) * _data->SO_PORTI * _data->SO_MERCI, 0600);
	_data_supply_demand = shmat(id, NULL, 0);
	_data->id_supply_demand = id;

	/* SHM: Ship */
	id = shmget(IPC_PRIVATE, sizeof(*_data_ship) * _data->SO_NAVI, 0600);
	_data_ship = shmat(id, NULL, 0);
	_data->id_const_ship = id;

	/* SHM: Cargo */
	id = shmget(IPC_PRIVATE, sizeof(*_data_cargo) * _data->SO_MERCI, 0600);
	_data_cargo = shmat(id, NULL, 0);
	_data->id_const_cargo = id;

	/* MSG: ports in */
	id = msgget(IPC_PRIVATE, 0600);
	_data->id_msg_in_ports = id;

	/* MSG: ports out */
	id = msgget(IPC_PRIVATE, 0600);
	_data->id_msg_out_ports = id;

	/* SEM: Start */
	id = semget(KEY_SEM, 1, 0600 | IPC_CREAT | IPC_EXCL);
	_id_sem=id;
	semctl(id, 0, SETVAL, 1);

	/* SEM: Docks */
	id = semget(IPC_PRIVATE, _data->SO_PORTI, 0600);
	_data->id_sem_docks = id;
}

void loop()
{
	while (1){
		dprintf(1, "[Parent] Wait\n");
		pause(); /* TODO get data */
	}
}

void create_children()
{
	/* Constants for readability */
	const SO_DAYS = _data->SO_DAYS;
	const SO_PORTI =  _data->SO_PORTI;
	const SO_FILL = _data->SO_FILL;
	const SO_LATO = _data->SO_LATO;
	const SO_NAVI = _data->SO_NAVI;
	const SO_BANCHINE = _data->SO_BANCHINE;
	const SO_MERCI = _data->SO_MERCI;
	const SO_SIZE = _data->SO_SIZE;
	const SO_MIN_VITA = _data->SO_MIN_VITA;
	const SO_MAX_VITA = _data->SO_MAX_VITA;

	int i, to_add, daily, n_docks;
	struct const_port *current_port;
	struct const_ship *current_ship;
	struct const_cargo *current_cargo;

	/* Initialize ports data */
	daily = SO_FILL / (SO_DAYS * SO_PORTI);
	for (i = 0; i < SO_PORTI; i++){
		current_port = &_data_port[i];
		current_port->pid = create_proc("./port", i);

		to_add = (i < (SO_FILL % (SO_DAYS * SO_PORTI))) ? 1 : 0;
		current_port->daily_restock_capacity = daily + to_add;

		if (i<4){
			/* ports in 4 corner */
			current_port->x = i % 2 != 0 ? SO_LATO : 0;
			current_port->y = i < 2 ? SO_LATO : 0;
		}
		else{
			current_port->x = get_random_coord();
			current_port->y = get_random_coord();
		}

		n_docks = get_random(1, SO_BANCHINE);
		semctl(_data->id_sem_docks, i, SETVAL, n_docks);
	}

	/* Initialize ships data */
	for (i = 0; i < SO_NAVI; i++){
		current_ship = &_data_ship[i];
		current_ship->pid = create_proc("./ship", i);

		current_ship->x = get_random_coord();
		current_ship->y = get_random_coord();
		current_ship->is_moving = FALSE;
	}

	/* Initialize cargo data */
	for (i = 0; i < SO_MERCI; i++){
		current_cargo = &_data_cargo[i];

		current_cargo->weight_batch = get_random(1, SO_SIZE);
		current_cargo->shelf_life = get_random(SO_MIN_VITA, SO_MAX_VITA);
	}

	/* Initialize supply and demand */
	bzero(_data_supply_demand, sizeof(*_data_supply_demand) * _data->SO_PORTI * _data->SO_MERCI);
}

double get_random_coord()
{
	return rand() / (double)INT_MAX * _data->SO_LATO;
}

int get_random(int min, int max)
{
	return rand() % (max - min) + min;
}

void read_constants_from_file()
{
	FILE *file;
	file = fopen("constants.txt", "r");

	/*Reading*/
	fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			/* Generic simulation specifications */
			&_data->SO_LATO, &_data->SO_DAYS,
			&_data->SO_NAVI, &_data->SO_PORTI, &_data->SO_MERCI,
			/* Weather events max duration */
			&_data->SO_STORM_DURATION, &_data->SO_SWELL_DURATION, &_data->SO_MAELSTORM,
			/* Ports specifications */
			&_data->SO_FILL, &_data->SO_BANCHINE, &_data->SO_LOADSPEED,
			/* Ships specifications */
			&_data->SO_SIZE, &_data->SO_SPEED, &_data->SO_CAPACITY,
			/* Cargo specifications */
			&_data->SO_MIN_VITA, &_data->SO_MAX_VITA);

	fclose(file);
}

pid_t create_proc(char *name, int index)
{
	pid_t proc_pid;
	char *arg[3], *env[]={NULL}, buf[10];

	if ((proc_pid = fork()) == -1){
		close_all("[FATAL] Couldn't fork", EXIT_FAILURE);
	} else if (proc_pid == 0){
		sprintf(buf, "%d", index);
		arg[0] = name;
		arg[1] = buf;
		arg[2] = NULL;

		execve(name, arg, env);
		dprintf(1, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	return proc_pid;
}

void custom_handler(int signal)
{
	switch (signal){
	case SIGTERM:
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	int i;

	/* Killing and wait child */
	for (i = 0; i<_data->SO_PORTI; i++){
		if(_data_port[i].pid > 0)
			kill(_data_port[i].pid, SIGTERM);
	}
	for (i = 0; i < _data->SO_NAVI; i++){
		if(_data_ship[i].pid > 0)
			kill(_data_ship[i].pid, SIGTERM);
	}

	while(wait(NULL) != -1 || errno == EINTR);

	/* Removing semaphors */
	semctl(_id_sem, 0, IPC_RMID);
	semctl(_data->id_sem_docks, 0, IPC_RMID);

	/* Closing message queue (need to be done before detaching shm) */
	msgctl(_data->id_msg_in_ports, IPC_RMID, NULL);
	msgctl(_data->id_msg_out_ports, IPC_RMID, NULL);

	/* Detach and mark for removal IPC structures (needed in this order) */
	shmctl(id_data, IPC_RMID, NULL);
	shmctl(_data->id_const_port, IPC_RMID, NULL);
	shmctl(_data->id_const_ship, IPC_RMID, NULL);
	shmctl(_data->id_const_cargo, IPC_RMID, NULL);
	shmctl(_data->id_supply_demand, IPC_RMID, NULL);
	detach(_data);
	detach(_data_port);
	detach(_data_ship);
	detach(_data_supply_demand);
	detach(_data_cargo);

	/* Messanges and exit */
	if (exit_status == EXIT_SUCCESS)
		dprintf(1, "\n%s\n", message);
	else
		dprintf(2, "\n%s\n", message);
	exit(exit_status);
}
