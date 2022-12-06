#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#define KEY_SEM 0xa2

/* Prototype */
struct sembuf create_sembuf(int, int);

#endif
