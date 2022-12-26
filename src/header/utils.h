#ifndef _UTILS_H
#define _UTILS_H

#include <signal.h>
#include <sys/time.h>

/* User defined signal */
#define SIGDAY SIGCHLD
#define SIGSWELL SIGUSR1
#define SIGSTORM SIGUSR1
#define SIGMAELSTROM SIGUSR2

/* Type (boolean and id) */
typedef int bool_t;
#define TRUE 1
#define FALSE 0
typedef int id_shared_t;

/* Coordinates */
struct coord{
	double x;
	double y;
};

/* Cargo list */
typedef struct{
	struct node_cargo *first;
} list_cargo;

/* Prototype */
void remove_cargo(list_cargo *list, int amount);
void add_cargo(list_cargo *list, int amount, int expiry_date);
int count_cargo(list_cargo *list);
void pop_cargo(list_cargo *list, int *amount, int *expiry_date);
int remove_expired_cargo(list_cargo *list, int today);
int get_not_expired_by_day(list_cargo *list, int day);
void free_cargo(list_cargo *list);

struct timespec get_timespec(double interval_sec);
void timer(double interval_sec);
void wait_event_duration(double sec);

/* MACRO FUNCTION */
#define TEST() dprintf(1, "TEST ALIVE (pid: %d): is still alive at %d in %s\n", getpid(), __LINE__, __FILE__)
#define GET_SIGN(number)\
	((number) == 0 ? 0 : ((number) > 0 ? 1 : -1))
#define SEND_SIGNAL(pid, signal)\
	if((pid) > 1 && (pid) != getpid()) kill((pid), (signal))
#define RANDOM(min_included, max_excluded)\
	((min_included) == (max_excluded) ? (min_included)\
	: rand() % ((max_excluded) - (min_included)) + (min_included))
#define RANDOM_DOUBLE(min_included, max_excluded)\
	(rand() / (double)INT_MAX * (max_excluded - min_included) + (min_included))

#endif
