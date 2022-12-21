#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
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

struct timespec get_timespec(double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * pow(10, 9);

	return res;
}

void timer(double interval_sec){
	struct itimerval interval;
	long sec, usec;

	sec = (long)interval_sec;
	usec = (interval_sec - (long)interval_sec) * pow(10, 6);

	interval.it_interval.tv_sec = sec;
	interval.it_interval.tv_sec = usec;
	interval.it_value = interval.it_interval;

	setitimer(ITIMER_REAL, &interval, NULL);
}
