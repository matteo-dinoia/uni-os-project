
/* Libraries */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

/* Macros */
#define SHIP_EXEC "./ship"
#define PORT_EXEC "./port"

/* Global variables */
pid_t *_proc_pid_list;

/* Prototypes */
pid_t create_proc(char **);
void custom_handler(int);
void close_all(const char *message, int exit_status);

int main()
{

	/* Will likely read contants from file using scanf() */
	//char *args[] = {SHIP_EXEC,};
}

pid_t create_proc(char **args)
{
	pid_t proc_pid;
	if((proc_pid = fork()) == -1) {
		close_all("[FATAL] Couldn't fork", EXIT_FAILURE);
	} else if (proc_pid == 0){
		execve(args[0], args, NULL);
		close_all("[FATAL] Failed to run executable", EXIT_FAILURE);
	}

	return proc_pid;
}

void custom_handler(int signal)
{
	switch (signal) {
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	if(exit_status == EXIT_SUCCESS) {
		dprintf(1, "%s\n", message);
	} else {
		dprintf(2, "%s\n", message);
	}
	exit(exit_status);
}
