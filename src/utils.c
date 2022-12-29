#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "header/utils.h"

/* Sorted by expiry date */
struct node_cargo{
	int amount;
	int expiry_date;
	struct node_cargo *next;
};

void _print_cargo(list_cargo *list){
	struct node_cargo *el;

	dprintf(1, "[CARGO_LIST %p] ", (void *) list);
	if(list == NULL){
		dprintf(1, "List is null\n");
		return;
	}

	for(el = list->first; el != NULL; el = el->next)
		dprintf(1, "q%de%d ", el->amount, el->expiry_date);
	dprintf(1, "\n");
}

void remove_cargo(list_cargo *list, int amount)
{
	struct node_cargo *current = list->first, *tmp;

	while (amount > 0){
		if (current == NULL) {
			dprintf(1, "Should have controlled NULL in remove cargo.\n");
			return;
		}

		if (amount >= current->amount){
			amount -=  current->amount;
			tmp = current;

			current = current->next;
			free(tmp);
		}
		else{
			amount = 0;
			current->amount -= amount;
		}
	}

	list->first = current;
}

void add_cargo(list_cargo *list, int amount, int expiry_date)
{
	struct node_cargo *prev, *tmp, *current;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in add cargo.\n");
		return;
	}

	/* Is to be added on the head */
	if(list->first == NULL || list->first->expiry_date >= expiry_date){
		/* Create new first */
		tmp = list->first;
		list->first = malloc(sizeof(*list->first));
		list->first->expiry_date = expiry_date;
		list->first->amount = amount;
		list->first->next = tmp;
		return;
	}

	/* Is in the middle */
	prev = list->first;
	do {
		current = prev->next;
		if(current == NULL || current->expiry_date >= expiry_date){
			tmp = current;

			/* Create new node */
			current = malloc(sizeof(*prev));
			current->expiry_date = expiry_date;
			current->amount = amount;
			current->next = tmp;

			/* Attach it and end cycle*/
			prev->next = current;
			current = NULL;
		}
		prev = prev->next;
	} while (current != NULL);
}

int count_cargo(list_cargo *list)
{
	struct node_cargo *current = list->first;
	int res = 0;

	while (current != NULL){
		res +=  current->amount;
		current = current->next;
	}

	return res;
}

void pop_cargo(list_cargo *list, int *amount, int *expiry_date)
{
	struct node_cargo *tmp;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in pop cargo (list = %p).\n", (void *) list);
		*amount = 0;
		*expiry_date = -14;
		return;
	}else if(list->first == NULL){
		dprintf(1, "Should have controlled NOT_EMPTY in pop cargo (list-> first = %p).\n", (void *) list->first);
		*amount = 0;
		*expiry_date = -3;
		return;
	}

	*amount = list->first->amount;
	*expiry_date = list->first->expiry_date;

	tmp = list->first;
	list->first = list->first->next;
	free(tmp);
}

int remove_expired_cargo(list_cargo *list, int today)
{
	struct node_cargo *tmp;
	int amount_removed = 0;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in remove expired cargo (list = %p).\n", (void *) list);
		return 0;
	}

	for(tmp = list->first; tmp != NULL; ){
		if (tmp->expiry_date > today)
			break; /* if not expired*/

		amount_removed += tmp->amount;
		list->first = tmp->next;
		free(tmp);
		tmp = list->first;
	}

	return amount_removed;
}

int get_not_expired_by_day(list_cargo *list, int day)
{
	struct node_cargo *tmp;
	int res = 0;

	if(list == NULL){
		dprintf(1, "Should have controlled NULL in get not expired by day (list = %p).\n", (void *) list);
		return 0;
	}

	for(tmp = list->first; tmp != NULL; tmp = tmp->next){
		if (tmp->expiry_date > day) /* if not expired*/
			res += tmp->amount;
	}

	return res;
}

void free_cargo(list_cargo *list)
{
	struct node_cargo *tmp;
	if (list == NULL) return;

	for(tmp = list->first; tmp != NULL; ){
		list->first = tmp->next;
		free(tmp);
		tmp = list->first;
	}
}

struct timespec _get_timespec(double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * pow(10, 9);

	return res;
}

void timer(double duration_sec){
	struct itimerval it_val;
	int sec, usec, ret;

	sec = (int) duration_sec;
	usec = (duration_sec - (int)duration_sec) * pow(10, 6);

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
