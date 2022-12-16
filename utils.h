#ifndef _UTILS_H
#define _UTILS_H

typedef struct _list_cargo list_cargo;

/* Prototype */
void remove_cargo(list_cargo *, int);
void add_cargo(list_cargo *, int, int);
int count_cargo(list_cargo *);
struct timespec get_timespec(double);


#endif
