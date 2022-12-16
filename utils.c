#include "utils.h"
#include <stddef.h>

/* Sorted by expiry date */
struct _list_cargo
{
	struct node_cargo *first;
};

struct node_cargo{
	int amount;
	int expiry_date;
	struct node_cargo *next;
};

void remove_cargo(list_cargo *list, int n)
{
	struct node_cargo *current = list->first, *to_remove;

	while (n > 0){
		if (current == NULL) {
			dprintf(1, "FUCK should have controlled NULL in remove cargo. kys\n");
			return;
		}

		if (n >= current->amount){
			n -=  current->amount;
			to_remove = current;
			current = current->next;
			free(to_remove);
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
	struct node_cargo *current, to_move;

	if(list == NULL){
		dprintf(1, "FUCK should have controlled NULL in add cargo. kys\n");
		return;
	}
	current = list->first;

	if(list->first == NULL || list->first->expiry_date >= expiry_date){
		/* Create new first */
		list->first = malloc(*list->first);
		list->first->expiry_date = expiry_date;
		list->first->amount = amount;
		list->first->next = current;
		return;
	}

	do {
		current = current->next;
		if(current == NULL || current->expiry_date >= expiry_date){
			to_move = current;
			current = malloc(*current);
			current->expiry_date = expiry_date;
			current->amount = amount;
			current->next = to_move;
			current = null;
		}
	} while (current != null);
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

struct timespec get_timespec(double interval_sec){
	struct timespec res;

	res.tv_sec = (long)interval_sec;
	res.tv_nsec = (interval_sec - (long)interval_sec) * pow(10, 9);

	return res;
}
