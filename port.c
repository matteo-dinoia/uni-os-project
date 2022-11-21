#define _GNU_SOURCE

/* Libraries */
#include <stdlib.h>
#include <signal.h>
#include <string.h>

/* Global variables */
int port_id;


/* Prototypes */
void supply_demand_update();
void signal_handler(int);

int main(int argc, char *argv[])
{
	/* Variables */
	struct sigaction sa;

	/* Setting singal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	/* TODO check for invalid arguments */
	port_id = atoi(argv[1]);

	supply_demand_update();
}

void supply_demand_update()
{

}

void signal_handler(int signal)
{
	switch (signal) {
	case SIGUSR1: /* Change of day */
		supply_demand_update();
		break;
	case SIGUSR2: /* Seastorm */
		supply_demand_update();
		break;
	}
}