
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
struct simulation_constant *_constants;

/* Prototypes */
pid_t create_proc();
void custom_handler(int);
void close_all(const char *message, int exit_status);
void create_children();
void read_constants_from_file();
void create_shared_structures();

int main()
{
	create_shared_structures();
	read_constants_from_file();

	create_children();
}

void create_shared_structures(){
	int id;
	id=shmget(KEY_SHARED, sizeof(*_constants), 0600);
	_constants=shmat(id, NULL, 0);
}

void read_constants_from_file(){
	FILE *file;
	file = fopen("constants.txt", "r");

	scanf(file, "%d", &(_constants->so_merci));
	scanf(file, "%d", &(_constants->so_fill));
	scanf(file, "%d", &(_constants->so_days));
	scanf(file, "%d", &(_constants->so_navi));
	scanf(file, "%d", &(_constants->so_porti));
	scanf(file, "%d", &(_constants->so_merci));
	scanf(file, "%d", &(_constants->so_size));
	scanf(file, "%d", &(_constants->so_min_vita));
	scanf(file, "%d", &(_constants->so_max_vita));
	scanf(file, "%d", &(_constants->so_lato));
	scanf(file, "%d", &(_constants->so_speed));
	scanf(file, "%d", &(_constants->so_capacity));
	scanf(file, "%d", &(_constants->so_banchine));
	scanf(file, "%d", &(_constants->so_loadspeed));
	scanf(file, "%d", &(_constants->so_storm_duration));
	scanf(file, "%d", &(_constants->so_swell_duration));
	scanf(file, "%d", &(_constants->so_maelestorm));
}

void create_children(){
	int i;

	_port_pid_list = calloc(_constants->so_porti, sizeof(*_port_pid_list));
	for(i = 0; _constants->so_porti; i++)
		_port_pid_list[i] = create_proc("port");


	_ship_pid_list = calloc(_constants->so_navi, sizeof(*_ship_pid_list));
	for(i = 0;i<_constants->so_navi; i++)
		_ship_pid_list[i] = create_proc("ship");
}

pid_t create_proc(char * name)
{
	pid_t proc_pid;
	if((proc_pid = fork()) == -1) {
		close_all("[FATAL] Couldn't fork", EXIT_FAILURE);
	} else if (proc_pid == 0){
		execve(name, (char**){name, NULL}, (char**){NULL});
		close_all("[FATAL] Failed to run executable", EXIT_FAILURE);
	}

	return proc_pid;
}

void custom_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	if(exit_status == EXIT_SUCCESS) {
		dprintf(1, "%s\n", message);
	} else {
		dprintf(2, "%s\n", message);
	}
	exit(exit_status);
}
