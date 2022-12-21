#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "header/semaphore.h"
#include "header/shared_mem.h"
#include "header/utils.h"

/* Macros */

/* Global Variables */
/* Shared Memory */
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;

/* Prototype */
void storm();
void maelstrom();
void swell();
void signal_handler(int);
void close_all();

int main()
{
	/* Variables */
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
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGDAY, &sa, NULL);
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGINT, &sa, NULL);

	/* LAST: Start running */
	srand(time(NULL) * getpid());
	timer(_data->SO_MAELSTROM / 24.0);

	/* Wait Forever */
	dprintf(1, "[Meteo] Wait\n");
	while (1) pause();
}

/* Adds SO_STORM_DURATION to the ship travel time once per day */
void storm()
{
	const SO_NAVI = _data->SO_NAVI;
	int i;
	int ship_i = RANDOM(0, SO_NAVI);

	for (i = 0; i < SO_NAVI; i++){
		if (_data_ship[ship_i].pid != 0 && _data_ship[ship_i].is_moving){
			kill(_data_ship[ship_i].pid, SIGSTORM);
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
	int ship_i = RANDOM(0, SO_NAVI);

	for (i = 0; i < SO_NAVI; i++){
		if (_data_ship[ship_i].pid != 0){
			kill(_data_ship[ship_i].pid, SIGMAELSTROM);
			return;
		}
		ship_i = (ship_i + 1) % SO_NAVI;
	}

	/* Comunicate all ship are dead to parent */
	kill(getppid(), SIGTERM); /* TODO: change signale */
}

/* Stop a port every day for SO_SWELL_DURATION hours */
void swell()
{
	const SO_PORTI = _data->SO_PORTI;
	int port_i = RANDOM(0, SO_PORTI);

	kill(_data_port[port_i].pid, SIGSWELL);
}

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		close_all();
	case SIGDAY: /* Change of day */
		/* storm();
		swell(); */
		break;
	case SIGALRM: /* Swell */
		dprintf(1, "TEST WEATHER\n");
		/* maelstrom(); */
		break;
	}
}

void close_all()
{
	detach(_data);
	detach(_data_port);
	detach(_data_ship);

	exit(0);
}