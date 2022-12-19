#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>

/* Macros */

/* Global Variables */
/* Shared Memory */

/* Prototype */
void loop();

int main(){
	/* Initializing */
	srand(time(NULL) * getpid());

	loop();
}

void loop(){
	while (1){
		dprintf(1, "[Meteo] Wait\n");
		pause(); /* TODO do meteo stuff */
	}
}