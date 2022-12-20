#ifndef _UTILS_H
#define _UTILS_H

/* MACRO */
#define TEST() dprintf(1, "TEST ALIVE: is still alive at %d in %s", __LINE__, __FILE__)
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
int get_random(int min_included, int max_excluded);

#endif
