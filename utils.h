#ifndef _UTILS_H
#define _UTILS_H

typedef struct{
	struct node_cargo *first;
} list_cargo;

/* Prototype */
void remove_cargo(list_cargo *, int);
void add_cargo(list_cargo *, int, int);
int count_cargo(list_cargo *);
void pop_cargo(list_cargo *, int *, int *);
struct timespec get_timespec(const double);


#endif
