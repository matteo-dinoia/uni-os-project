/* MAY RENAME TO DATA*/
/* Libraries */
#include <stdlib.h>
#include <string.h>
#include "base.h"

/* Macros */

/* Global variables */

/* Prototypes */

void initialize_shared(struct shared_pointer *pointers, struct simulation_constant *gen_const)
{
	int id;

	id = shmget(KEY_SHARED, sizeof(*pointers->general_constants), 0600);
	pointers->general_constants = shmat(id, NULL, 0);
	memcpy(pointers->general_constants, gen_const, sizeof(*gen_const));
	gen_const = pointers->general_constants;

	id = shmget(IPC_PRIVATE, sizeof(*gen_const->ports_constant) * gen_const->so_porti, 0600);
	gen_const->shared_port = id;
	gen_const->ports_constant = shmat(id, NULL, 0);
}
