#define _GNU_SOURCE
#include "utils.h"
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

/* Sorted by expiry date */
struct node_cargo{
	int amount;
	int expiry_date;
	struct node_cargo *next;
};

void remove_cargo(list_cargo *list, int amount)
{
	struct node_cargo *current = list->first, *to_remove;

	while (amount > 0){
		if (current == NULL) {
			dprintf(1, "FUCK should have controlled NULL in remove cargo. kys\n");
			return;
		}

		if (amount >= current->amount){
			amount -=  current->amount;
			to_remove = current;
			current = current->next;
			free(to_remove);
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
	struct node_cargo *current, *to_move;

	if(list == NULL){
		dprintf(1, "FUCK should have controlled NULL in add cargo. kys\n");
		return;
	}
	current = list->first;

	if(list->first == NULL || list->first->expiry_date >= expiry_date){
		/* Create new first */
		list->first = malloc(sizeof(*list->first));
		list->first->expiry_date = expiry_date;
		list->first->amount = amount;
		list->first->next = current;
		return;
	}

	do {
		current = current->next;
		if(current == NULL || current->expiry_date >= expiry_date){
			to_move = current;
			current = malloc(sizeof(*current));
			current->expiry_date = expiry_date;
			current->amount = amount;
			current->next = to_move;
			current = NULL;
		}
	} while (current != NULL);
}

int count_cargo(list_cargo *list)
{
	struct node_cargo *current = list->first;
	int res;

	while (current != NULL){
		res +=  current->amount;
		current = current->next;
	}

	return res;
}

void pop_cargo(list_cargo *list, int *amount, int *expiry_date)
{
	struct node_cargo *to_remove;
	if(list == NULL || list->first){
		dprintf(1, "FUCK should have controlled NULL in pop cargo. kys\n");
		return;
	}

	*amount = list->first->amount;
	*expiry_date = list->first->expiry_date;

	to_remove =list->first;
	list->first = list->first->next;
	free(to_remove);
}

struct timespec get_timespec(double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * pow(10, 9);

	return res;
}
