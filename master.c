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

/* Macros */
#define SHIP_EXEC "./ship"
#define PORT_EXEC "./port"

/* Global variables */

/* Shared memory */
struct const_general *_data;
struct const_port *_data_port;
struct const_ship *_data_ship;
int id_data;

/* Prototypes */
pid_t create_proc(char *, int);
void custom_handler(int);
void close_all(const char *, int);
void create_children();
void read_constants_from_file();
void create_shared_structures();
double get_random_coord();
void loop();

int main()
{
	int id;
	struct sigaction sa;
	sigset_t set_masked;
	struct sembuf sem_oper;

	srand(time(NULL) * getpid());

	/* General */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600 | IPC_CREAT);
	id_data = id;
	_data = shmat(id, NULL, 0);
	shmctl(id_data, IPC_RMID, NULL);
	read_constants_from_file();

	/* Port */
	id = shmget(IPC_PRIVATE, sizeof(*_data_port) * _data->SO_PORTI, 0600);
	_data_port = shmat(id, NULL, 0);
	_data->id_const_port = id;
	shmctl(_data->id_const_port, IPC_RMID, NULL);

	/* Ship */
	id = shmget(IPC_PRIVATE, sizeof(*_data_ship) * _data->SO_NAVI, 0600);
	_data_ship = shmat(id, NULL, 0);
	_data->id_const_ship = id;
	shmctl(_data->id_const_ship, IPC_RMID, NULL);

	/* TODO Initializing message queue for ports*/

	create_children();

	/* LAST: Start child */
	id = semget(KEY_SHARED, 1, 0600);
	sem_oper = create_sembuf(0, 0);
	semop(id, &sem_oper, 1);

	/* LAST: Setting signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &custom_handler;
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* LAST: Start running*/
	loop();
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
	int i, to_add, daily;
	struct const_port *current_port;
	struct const_ship *current_ship;

	daily = _data->SO_FILL / (_data->SO_DAYS * _data->SO_PORTI);
	for (i = 0; i < _data->SO_PORTI; i++){
		current_port = &_data_port[i];
		current_port->pid = create_proc("./port", i);

		to_add = (i < (_data->SO_FILL % (_data->SO_DAYS * _data->SO_PORTI))) ? 1 : 0;
		current_port->daily_restock_capacity = daily + to_add;

		if (i<4){
			/* ports in 4 corner */
			current_port->x = i % 2 != 0 ? _data->SO_LATO : 0;
			current_port->y = i < 2 ? _data->SO_LATO : 0;
		}
		else{
			current_port->x = get_random_coord();
			current_port->y = get_random_coord();
		}
	}

	for (i = 0; i < _data->SO_NAVI; i++){
		current_ship = &_data_ship[i];
		current_ship->pid = create_proc("./ship", i);

		current_ship->x = get_random_coord();
		current_ship->y = get_random_coord();
		current_ship->is_moving = FALSE;
	}
}

double get_random_coord(){
	return rand() / (double)INT_MAX * _data->SO_LATO;
}

void read_constants_from_file()
{
	FILE *file;
	file = fopen("constants.txt", "r");

	/* Reding generic simulation specifications */
	fscanf(file, "%d", &_data->SO_LATO);
	fscanf(file, "%d", &_data->SO_DAYS);
	fscanf(file, "%d", &_data->SO_NAVI);
	fscanf(file, "%d", &_data->SO_PORTI);
	fscanf(file, "%d", &_data->SO_MERCI);

	/* Reading weather events max duration */
	fscanf(file, "%d", &_data->SO_STORM_DURATION);
	fscanf(file, "%d", &_data->SO_SWELL_DURATION);
	fscanf(file, "%d", &_data->SO_MAELSTORM);

	/* Reading ports specifications */
	fscanf(file, "%d", &_data->SO_FILL);
	fscanf(file, "%d", &_data->SO_BANCHINE);
	fscanf(file, "%d", &_data->SO_LOADSPEED);

	/* Reading ships specifications */
	fscanf(file, "%d", &_data->SO_SIZE);
	fscanf(file, "%d", &_data->SO_SPEED);
	fscanf(file, "%d", &_data->SO_CAPACITY);

	/* Reading cargo specifications */
	fscanf(file, "%d", &_data->SO_MIN_VITA);
	fscanf(file, "%d", &_data->SO_MAX_VITA);
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
		dprintf(1, "[Child %d] Starting with \"%s\"\n", getpid(), arg[1]);

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

	/* Detach and mark for removal IPC structures */
	detach(_data);
	detach(_data_port);
	detach(_data_ship);


	/* Messanges and exit */
	if (exit_status == EXIT_SUCCESS)
		dprintf(1, "\n%s\n", message);
	else
		dprintf(2, "\n%s\n", message);
	exit(exit_status);
}
