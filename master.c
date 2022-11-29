
/* Libraries */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/shm.h>
#include "base.h"

/* Macros */
#define SHIP_EXEC "./ship"
#define PORT_EXEC "./port"

/* Global variables */
pid_t *_ship_pid_list;
pid_t *_port_pid_list;
/* Shared memory */
struct const_general *_data;
struct const_port *_data_port;
struct const_port *_data_ship;
struct bump_general *_bump_general;
struct bump_ship *_bump_ship;
struct bump_port *_bump_port;
struct bump_port *_bump_cargo;

/* Prototypes */
pid_t create_proc();
void custom_handler(int);
void close_all(const char *, int);
void create_children();
void read_constants_from_file();
void create_shared_structures();

int main()
{
	/* TODO Initialize constant_general */
	read_constants_from_file();

	/* TODO Initialize other shared memory*/

	create_children(); /* TODO set memory value*/
}

void create_children()
{
	int i;

	_port_pid_list = calloc(_data->SO_PORTI, sizeof(*_port_pid_list));
	for (i = 0; _data->SO_PORTI; i++)
		_port_pid_list[i] = create_proc("port");

	_ship_pid_list = calloc(_data->SO_NAVI, sizeof(*_ship_pid_list));
	for (i = 0; i < _data->SO_NAVI; i++)
		_ship_pid_list[i] = create_proc("ship");
}

void read_constants_from_file()
{
	FILE *file;
	file = fopen("constants.txt", "r");

	/* Reding generic simulation specifications */
	scanf(file, "%d", &_data->SO_LATO);
	scanf(file, "%d", &_data->SO_DAYS);
	scanf(file, "%d", &_data->SO_NAVI);
	scanf(file, "%d", &_data->SO_PORTI);
	scanf(file, "%d", &_data->SO_MERCI);

	/* Reading weather events max duration */
	scanf(file, "%d", &_data->SO_STORM_DURATION);
	scanf(file, "%d", &_data->SO_SWELL_DURATION);
	scanf(file, "%d", &_data->SO_MAELSTORM);

	/* Reading ports specifications */
	scanf(file, "%d", &_data->SO_FILL);
	scanf(file, "%d", &_data->SO_BANCHINE);
	scanf(file, "%d", &_data->SO_LOADSPEED);

	/* Reading ships specifications */
	scanf(file, "%d", &_data->SO_SIZE);
	scanf(file, "%d", &_data->SO_SPEED);
	scanf(file, "%d", &_data->SO_CAPACITY);

	/* Reading cargo specifications */
	scanf(file, "%d", &_data->SO_MIN_VITA);
	scanf(file, "%d", &_data->SO_MAX_VITA);
}

pid_t create_proc(char *name)
{
	pid_t proc_pid;
	if ((proc_pid = fork()) == -1)
	{
		close_all("[FATAL] Couldn't fork", EXIT_FAILURE);
	}
	else if (proc_pid == 0)
	{
		execve(name, (char **){name, NULL}, (char **){NULL});
		close_all("[FATAL] Failed to run executable", EXIT_FAILURE);
	}

	return proc_pid;
}

void custom_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	if (exit_status == EXIT_SUCCESS)
	{
		dprintf(1, "%s\n", message);
	}
	else
	{
		dprintf(2, "%s\n", message);
	}
	exit(exit_status);
}
