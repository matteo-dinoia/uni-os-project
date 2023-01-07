#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include "header/semaphore.h"

struct sembuf create_sembuf(int index, int value)
{
	struct sembuf res;

	res.sem_num = index;
	res.sem_op = value;
	res.sem_flg = SEM_UNDO; /* For avoiding leaving resource filled when process die*/

	return res;
}

void execute_single_sem_oper(int id, int index, int value){
	struct sembuf operation;

	operation = create_sembuf(index, value);
	while(semop(id, &operation, 1) == -1);
}
