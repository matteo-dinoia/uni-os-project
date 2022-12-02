/* Libraries */
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared_mem.h"

/* Macros */

/* Global variables */

/* Prototypes */

/*
 * clean_pointers()
 * create?()
 * attach(int value) using switch
 * deattach_all()
 */

void detach(void *pointer)
{
	if (pointer != NULL)
		shmdt(pointer);
}

id_shm_t get_shared(key_t key, size_t size)
{
	return shmget(key, size, 0600 | IPC_CREAT);
}

void *attach_shared(id_shm_t id)
{
	return shmat(id, NULL, 0);
}

struct sembuf create_sembuf(int index, int value)
{
	struct sembuf res;

	res.sem_num = index;
	res.sem_op = value;
	res.sem_flg = 0;

	return res;
}
