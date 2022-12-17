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

void remove_cargo(list_cargo *list, int n)
{
	struct node_cargo *current = list->first, *tmp;

	while (n > 0){
		if (current == NULL) {
			dprintf(1, "Should have controlled NULL in remove cargo.\n");
			return;
		}

		if (n >= current->amount){
			n -=  current->amount;
			tmp = current;
			current = current->next;
			free(tmp);
		}
		else{
			n = 0;
			current->amount -= n;
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
	int res;

	while (current != NULL){
		res +=  current->amount;
		current = current->next;
	}

	return res;
}

void pop_cargo(list_cargo *list, int *amount, int *expiry_date)
{
	struct node_cargo *tmp;
	if(list == NULL || list->first){
		dprintf(1, "Should have controlled NULL in pop cargo.\n");
		return;
	}

	*amount = list->first->amount;
	*expiry_date = list->first->expiry_date;

	tmp =list->first;
	list->first = list->first->next;
	free(tmp);
}

struct timespec get_timespec(const double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * pow(10, 9);

	return res;
}
