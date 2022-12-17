#ifndef _UTILS_H
#define _UTILS_H

typedef struct{
	struct node_cargo *first;
} list_cargo;

/* Prototype */
void remove_cargo(list_cargo *list, int amount);
void add_cargo(list_cargo *list, int amount, int expiry_date);
int count_cargo(list_cargo *list);
void pop_cargo(list_cargo *list, int *amount, int *expiry_date);
struct timespec get_timespec(double interval_sec);


#endif
