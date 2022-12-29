#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "header/semaphore.h"
#include "header/shared_mem.h"
#include "header/utils.h"
#include "header/shm_manager.h"

/* Prototypes */
void storm();
void maelstrom();
void swell();
void signal_handler(int);

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
	initialize_shm_manager(0, NULL);

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
	timer(SO_MAELSTROM / 24.0);

	/* Wait Forever */
	dprintf(1, "[Meteo] Wait\n");
	while (1) pause();
}

/* Adds SO_STORM_DURATION to the ship travel time once per day */
void storm()
{
	int i, rand_ship;
	rand_ship = RANDOM(0, SO_NAVI);
	int ship_i = get_ship_pid(rand_ship);
	for (i = 0; i < SO_NAVI; i++){
		if (ship_i != 0 && is_ship_moving(ship_i)){
			kill(ship_i, SIGSTORM);
			return;
		}
		rand_ship = (rand_ship + 1) % SO_NAVI;
		ship_i = get_ship_pid(rand_ship);
	}

	/* TODO: no ship is moving or all are dead */
}

/* Sink a ship every SO_MAELSTROM */
void maelstrom()
{
	int i, rand_ship;
	rand_ship = RANDOM(0, SO_NAVI);
	int ship_i = get_ship_pid(rand_ship);

	for (i = 0; i < SO_NAVI; i++){
		if (ship_i != 0){
			kill(ship_i, SIGMAELSTROM);
			return;
		}
		rand_ship = (rand_ship + 1) % SO_NAVI;
		ship_i = get_ship_pid(rand_ship);
	}

	/* Comunicate all ship are dead to parent */
	kill(getppid(), SIGTERM);
}

/* Stop a port every day for SO_SWELL_DURATION hours */
void swell()
{
	kill(get_port_pid(RANDOM(0, SO_PORTI)), SIGSWELL);
}

void signal_handler(int signal)
{
	switch (signal)
	{
	case SIGINT:
		close_shm_manager();
		exit(0);
	case SIGDAY: /* Change of day */
		storm();
		swell();
		break;
	case SIGALRM:
		dprintf(1, "TEST WEATHER\n");
		maelstrom();
		break;
	}
}
