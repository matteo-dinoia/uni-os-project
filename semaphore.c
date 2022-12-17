#define _GNU_SOURCE

#include <sys/sem.h>
#include "semaphore.h"

struct sembuf create_sembuf(int index, int value)
{
	struct sembuf res;

	res.sem_num = index;
	res.sem_op = value;
	res.sem_flg = 0;

	return res;
}

void execute_single_sem_oper(int id, int index, int value){
	struct sembuf operation;

	operation = create_sembuf(index, value);
	semop(id, &operation, 1);
}
