#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include <sys/sem.h>

#define KEY_SEM_START 0x110fff
#define KEY_SEM_CARGO 0x120fff
#define KEY_SEM_DOCKS 0x130fff

/* Prototype */
struct sembuf create_sembuf(int index, int value);
void execute_single_sem_oper(int id, int index, int value);

#endif
