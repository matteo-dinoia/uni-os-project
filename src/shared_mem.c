#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include "header/shared_mem.h"

/* Macros */

/* Global variables */

/* Prototypes */

void detach(void *pointer)
{
	if (pointer != NULL)
		shmdt(pointer);
}

id_shared_t get_shared(key_t key, size_t size)
{
	return shmget(key, size, 0600 | IPC_CREAT);
}

void *attach_shared(id_shared_t id, int extra_flags)
{
	return shmat(id, NULL, 0 | extra_flags);
}
