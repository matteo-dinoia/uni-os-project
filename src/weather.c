#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
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
	while (1) pause();
}

/* Adds SO_STORM_DURATION to the ship travel time once per day */
void storm()
{
	int i;
	int ship_i = RANDOM(0, SO_NAVI);
	for (i = 0; i < SO_NAVI; i++){
		if (!is_ship_dead(ship_i) && is_ship_moving(ship_i)){
			SEND_SIGNAL(get_ship_pid(ship_i), SIGSTORM);
			return;
		}
		ship_i = (ship_i + 1) % SO_NAVI;
	}

	/* TODO: no ship is moving*/
}

/* Sink a ship every SO_MAELSTROM */
void maelstrom()
{
	int i;
	int ship_i = RANDOM(0, SO_NAVI);

	for (i = 0; i < SO_NAVI; i++){
		if (!is_ship_dead(ship_i)){
			SEND_SIGNAL(get_ship_pid(ship_i), SIGMAELSTROM);
			return;
		}
		ship_i = (ship_i + 1) % SO_NAVI;
	}
}

/* Stop a port every day for SO_SWELL_DURATION hours */
void swell()
{
	SEND_SIGNAL(get_port_pid(RANDOM(0, SO_PORTI)), SIGSWELL);
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
		maelstrom();
		break;
	}
}
