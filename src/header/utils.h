#ifndef _UTILS_H
#define _UTILS_H

#include <signal.h>

/* MACRO */
#define TEST() dprintf(1, "TEST ALIVE: is still alive at %d in %s", __LINE__, __FILE__)
/* User defined signal */
#define SIGDAY SIGCHLD
#define SIGSWELL SIGUSR1
#define SIGSTORM SIGUSR1
#define SIGMAELSTROM SIGUSR2

/* Fake boolean */
#define TRUE 1
#define FALSE 0
typedef int bool_t;

/* Shared id */
typedef int id_shared_t;

/* Cargo list (TODO Move out of here)*/
typedef struct{
	struct node_cargo *first;
} list_cargo;

/* Prototype */
void remove_cargo(list_cargo *list, int amount);
void add_cargo(list_cargo *list, int amount, int expiry_date);
int count_cargo(list_cargo *list);
void pop_cargo(list_cargo *list, int *amount, int *expiry_date);

struct timespec get_timespec(double interval_sec);

/* MACRO FUNCTION */
#define SEND_SIGNAL(pid, signal)\
	if((pid) > 1 && (pid) != getpid()) kill((pid), (signal))
#define RANDOM(min_included, max_excluded)\
	(rand() % ((max_excluded) - (min_included)) + (min_included))
#define RANDOM_DOUBLE(min_included, max_excluded)\
	(rand() / (double)INT_MAX * (max_excluded) + (min_included))

#endif
