#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include "header/utils.h"

#define REM_ELEM(prev_next_dest, current)\
		do {\
			(prev_next_dest) = (current)->next;\
			free((current));\
			(current) = (prev_next_dest);\
		}while (0)

/* Sorted by expiry date */
struct node_cargo{
	int amount;
	int expiry_date;
	struct node_cargo *next;
};

void print_cargo(list_cargo *list){
	struct node_cargo *node;

	if(list == NULL){
		dprintf(1, "List is null\n");
		return;
	}else if (list->first == NULL){
		dprintf(1, "List has 0 elements\n");
		return;
	}

	dprintf(1, "LIST [Lenght: %d]: ", count_cargo(list));
	for(node = list->first; node != NULL; node = node->next)
		dprintf(1, "%d [scad. %d] ", node->amount, node->expiry_date);
	dprintf(1, "\n");
}

void remove_cargo(list_cargo *list, int amount)
{
	struct node_cargo *node;

	node = list->first;
	while (amount > 0){
		if (node == NULL) {
			dprintf(1, "Should have controlled NULL in remove cargo.\n");
			return;
		}

		if (amount >= node->amount){
			amount -= node->amount;
			REM_ELEM(list->first, node);
		}else{
			node->amount -= amount;
			amount = 0;
		}
	}
}

void add_cargo(list_cargo *list, int amount, int expiry_date)
{
	struct node_cargo *prev, *tmp, *node;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in add cargo.\n");
		return;
	}

	/* Is in the middle */
	prev = NULL;
	node = list->first;
	for (node = list->first; 1; node = node->next){
		if(node == NULL || node->expiry_date > expiry_date){
			/* Create new tmp */
			tmp = malloc(sizeof(*prev));
			tmp->expiry_date = expiry_date;
			tmp->amount = amount;
			tmp->next = node;

			/* Attach it and end cycle*/
			if(prev == NULL) list->first = tmp;
			else prev->next = tmp;
			break;
		}else if (node->expiry_date == expiry_date){
			node->amount += amount;
			break;
		}

		prev = node;
	}
}

int count_cargo(list_cargo *list)
{
	struct node_cargo *node;
	int res = 0;

	for (node = list->first; node != NULL; node = node->next)
		res +=  node->amount;

	return res;
}

void pop_cargo(list_cargo *list, int *amount, int *expiry_date)
{
	struct node_cargo *node;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in pop cargo (list = %p).\n", (void *) list);
		*amount = 0;
		return;
	}else if(list->first == NULL){
		dprintf(1, "Should have controlled NOT_EMPTY in pop cargo (list-> first = %p).\n", (void *) list->first);
		*amount = 0;
		return;
	}

	*amount = list->first->amount;
	*expiry_date = list->first->expiry_date;

	node = list->first;
	REM_ELEM(list->first, node);
}

int remove_expired_cargo(list_cargo *list, int today)
{
	struct node_cargo *node;
	int amount_removed = 0;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in remove expired cargo (list = %p).\n", (void *) list);
		return 0;
	}

	for(node = list->first; node != NULL; ){
		if (node->expiry_date > today)
			break; /* if not expired */

		amount_removed += node->amount;
		REM_ELEM(list->first, node);
	}

	return amount_removed;
}

int get_not_expired_by_day(list_cargo *list, int day)
{
	struct node_cargo *node;
	int res = 0;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in get not expired by day (list = %p).\n", (void *) list);
		return 0;
	}

	for(node = list->first; node != NULL; node = node->next){
		if (node->expiry_date > day) /* if not expired*/
			res += node->amount;
	}

	return res;
}

void free_cargo(list_cargo *list)
{
	struct node_cargo *node;
	if (list == NULL) return;

	for(node = list->first; node != NULL; ){
		REM_ELEM(list->first, node);
	}
}

struct timespec _get_timespec(double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * 1e9;

	return res;
}

void timer(double duration_sec){
	struct itimerval it_val;
	int sec, usec, ret;

	sec = (int) duration_sec;
	usec = (duration_sec - (int)duration_sec) * 1e6;

	it_val.it_value.tv_sec = sec;
	it_val.it_value.tv_usec = usec;
	it_val.it_interval = it_val.it_value;

	ret = setitimer(ITIMER_REAL, &it_val, NULL);
}

void wait_event_duration(double sec)
{
	struct timespec rem_time, event_time;

	if (sec <= 0) return;

	event_time = _get_timespec(sec);
	do {
		errno = EXIT_SUCCESS;
		nanosleep(&event_time, &rem_time);
		event_time = rem_time;
	}while (errno == EINTR);
}
