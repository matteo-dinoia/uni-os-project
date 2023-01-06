#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include "header/shared_mem.h"
#include "header/semaphore.h"
#include "header/message.h"
#include "header/utils.h"
#include "header/shm_manager.h"

/* Macros */
#define DAY_SEC 1

/* Global variables */
pid_t *childs_pid = NULL;
int childs_counter = 0;

/* Prototypes */
pid_t create_proc(char *, int);
void custom_handler(int);
void close_all(const char *, int);
void create_children();
struct general read_constants_from_file();
void send_to_all_childs();
void loop();

int main()
{
	struct sigaction sa;
	sigset_t set_masked;
	struct general constants;

	/* Initializing */
	srand(time(NULL) * getpid());
	constants = read_constants_from_file();
	initialize_shm_manager(PORT_WRITE | SHIP_WRITE | CARGO_WRITE | SHOP_WRITE, &constants);

	/* Setting signal handler */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &custom_handler;
	sigaction(SIGALRM, &sa, NULL);
	sa.sa_mask = set_masked;
	sigfillset(&set_masked);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* Create and start children*/
	childs_pid = calloc(SO_NAVI + SO_PORTI + 1, sizeof(*childs_pid));
	create_children();
	start_simulation();

	/* Wait forever */
	alarm(DAY_SEC);
	while (1) pause();
}

void create_children()
{
	int i;
	for(i = 0; i < SO_PORTI; i++)
		set_port_pid(i, create_proc("./port", i));
	for(i = 0; i < SO_NAVI; i++)
		set_ship_pid(i, create_proc("./ship", i));
	create_proc("./weather", -1);
}

pid_t create_proc(char *name, int index)
{
	pid_t proc_pid;
	char *arg[3], *env[]={NULL}, buf[10];

	if ((proc_pid = fork()) == -1){
		close_all("[FATAL] Failed to fork child", EXIT_FAILURE);
	} else if (proc_pid == 0){
		sprintf(buf, "%d", index);
		arg[0] = name;
		arg[1] = buf;
		arg[2] = NULL;

		execve(name, arg, env);
		dprintf(1, "%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	childs_pid[childs_counter++] = proc_pid;

	return proc_pid;
}

struct general read_constants_from_file()
{
	struct general res;
	const NUM_VALUE = 16;
	int num_read, counter;
	double value;
	char c;

	/* Take file from out of bin directory */
	FILE *file = fopen("../constants.txt", "r");
	if (file == NULL)
		close_all("[FATAL] Could not open file constants.txt", EXIT_FAILURE);

	dprintf(1, "[CONST VALUE]");
	counter = 0;
	while ((num_read = fscanf(file, "%lf", &value)) != EOF){
		if (num_read != 0){
			if (counter >= NUM_VALUE){
				fclose(file);
				close_all("[FATAL] Found too many number (reading file constant.txt)", EXIT_FAILURE);
			}else if (value <= 0){
				fclose(file);
				close_all("[FATAL] Invalid number [<= 0] (reading file constant.txt)", EXIT_FAILURE);
			}else if (counter <= 0){
				res.so_lato = value;
				dprintf(1, " %lf", value);
			}else {
				(&res.so_days)[counter - 1] = (int) value;
				dprintf(1, " %d", (int) value);
			}
			counter++;
		}

		fscanf(file, "%*[ \t]");
		if ((c = fgetc(file)) == '#'){
			fscanf(file, "%*[^\n]");
		}else if(!(c >= '0' && c <= '9')){
			fclose(file);
			close_all("[FATAL] Found invalid char (reading file constant.txt)", EXIT_FAILURE);
		}
	}
	dprintf(1, "\n");


	fclose(file);
	if(counter < NUM_VALUE)
		close_all("[FATAL] Found too few number (reading file constant.txt)", EXIT_FAILURE);

	return res;
}

void custom_handler(int signal)
{
	switch (signal){
	case SIGSEGV:
		close_all("[ERROR] Segmentation memory", EXIT_FAILURE);
	case SIGTERM:
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
	case SIGALRM:
		print_dump_data();
		if(check_ships_all_dead()) close_all("[INFO] Simulation terminated because all ships died", EXIT_SUCCESS);
		if(check_shop_termination_condition()) close_all("[INFO] Simulation terminated because the demand or the supply is zero in all ports", EXIT_SUCCESS);
		if(get_day() > SO_DAYS) close_all("[INFO] Simulation terminated because period passed", EXIT_SUCCESS);

		increase_day();
		send_to_all_childs(SIGDAY);
		alarm(DAY_SEC);
		break;
	}
}

void send_to_all_childs(int signal){
	int i;
	for(i = 0; i < childs_counter; i++)
		SEND_SIGNAL(childs_pid[i], signal);
}

void close_all(const char *message, int exit_status)
{
	/* Messanges and exit */
	dprintf(1, "\n\n%s\n", message);

	/* Killing childs */
	send_to_all_childs(SIGINT);

	/* Closing IPC and local */
	close_shm_manager();
	close_sem_and_msg();
	free(childs_pid);

	/* Waiting childs */
	while(wait(NULL) != -1 || errno == EINTR) errno = EXIT_SUCCESS;

	/* Messanges and exit */
	dprintf(1, "[CLOSING OPERATION] Success\n");
	exit(exit_status);
}
