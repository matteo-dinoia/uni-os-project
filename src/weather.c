#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "header/shared_mem.h"
#include "header/utils.h"

/* Macros */

/* Global Variables */
/* Shared Memory */
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;

/* Prototype */
void loop();

int main()
{
	/* Variables */
	struct sembuf sem_oper;
	struct sigaction sa;
	sigset_t set_masked;
	int id;

	/* FIRST: Wait for father */
	id = semget(KEY_SEM, 1, 0600);
	execute_single_sem_oper(id, 0, 0);

	/* FIRST: Gain data struct */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600);
	_data = attach_shared(id);
	_data_port = attach_shared(_data->id_port);
	_data_ship = attach_shared(_data->id_ship);

	/* LAST: Set signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &signal_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGDAY, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGALRM, &sa, NULL);

	/* LAST: Start running */
	srand(time(NULL) * getpid());
	loop();
}

void loop()
{
	struct itimerval interval;

	/* Maelstrom clock */
	interval.it_interval = get_timespec(_data->SO_MAELSTROM);
	interval.it_value = interval.it_interval;
	setitimer(ITIMER_REAL, &interval, NULL);

	/* Wait Forever */
	dprintf(1, "[Meteo] Wait\n");
	while (1) pause();
}

/* Adds SO_STORM_DURATION to the ship travel time once per day */
void storm()
{
	const SO_NAVI = _data->SO_NAVI;
	int i;
	int ship_i = get_random(0, SO_NAVI);

	for (i = 0; i < SO_NAVI; i++){
		if (_data_ship[ship_i]->pid != 0 && _data_ship[ship_i]->is_moving){
			kill(_data_ship->pid, SIGSTORM); /* TODO: change the signal */
			return;
		}
		ship_i = (ship_i + 1) % SO_NAVI;
	}

	/* TODO: no ship is moving or all are dead */
}

/* Sink a ship every SO_MAELSTROM */
void maelstrom()
{
	const SO_NAVI = _data->SO_NAVI;
	int i;
	int ship_i = get_random(0, SO_NAVI);

	for (i = 0; i < SO_NAVI; i++){
		if (_data_ship[ship_i]->pid != 0){
			kill(_data_ship->pid, SIGMAELSTROM); /* TODO: change the signal */
			return;
		}
		ship_i = (ship_i + 1) % SO_NAVI;
	}

	/* Comunicate all ship are dead to parent */
	kill(_data->parent_pid, SIGTERM); /* TODO: change signale */
}

/* Stop a port every day for SO_SWELL_DURATION hours */
void swell()
{
	const SO_PORTI = _data->SO_PORTI;
	int port_i = get_random(0, SO_PORTI);

	kill(_data_ship->pid, SIGSWELL); /* TODO: change the signal */
}

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		close_all();
	case SIGDAY: /* Change of day */
		storm();
		swell();
		break;
	case SIGALRM: /* Swell */
		maelstrom();
		break;
	}
}
