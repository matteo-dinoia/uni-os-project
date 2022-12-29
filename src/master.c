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
id_shared_t _id_sem = -1;
id_shared_t _id_sem_cargo = -1;
id_shared_t _id_sem_docks = -1;
id_shared_t _id_msg_in_ports = -1;
id_shared_t _id_msg_out_ports = -1;

/* Prototypes */
void initialize_shared();
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
	int id_sem;

	/* Initializing Variable*/
	srand(time(NULL) * getpid());
	sigfillset(&set_masked);

	/* Setting signal handler (need to be done after) */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &custom_handler;
	sigaction(SIGALRM, &sa, NULL);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);

	/* Initializing no error can be inside */
	initialize_shared();

	/* Create and start children*/
	id_sem = semget(KEY_SEM, 1, 0600 | IPC_CREAT | IPC_EXCL);
	semctl(id_sem, 0, SETVAL, 1);
	create_children();
	semctl(id_sem, 0, SETVAL, 0);

	/* Wait forever */
	alarm(DAY_SEC);
	while (1) pause();
}

void initialize_shared()
{
	struct general constants;

	/* Initialize SHM manager */
	constants = read_constants_from_file();
	initialize_shm_manager(PORT_WRITE | SHIP_WRITE | CARGO_WRITE | SHOP_WRITE, &constants);

	/* MSG port in and out */
	_id_msg_in_ports = msgget(IPC_PRIVATE, 0600);
	_id_msg_out_ports = msgget(IPC_PRIVATE, 0600);

	/* SEM: Start and docks */
	_id_sem = semget(KEY_SEM, 1, 0600 | IPC_CREAT | IPC_EXCL);
	_id_sem_docks = semget(IPC_PRIVATE, SO_PORTI, 0600);
	_id_sem_cargo = semget(IPC_PRIVATE, SO_MERCI, 0600);

}

void create_children()
{
	int i;
	for(i = 0; i < SO_PORTI; i++)
		create_proc("./port", i);
	for(i = 0; i < SO_NAVI; i++)
		create_proc("./ship", i);
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
}

void custom_handler(int signal)
{
	int i;

	switch (signal){
	case SIGSEGV:
		close_all("[ERROR] Segmentation memory", EXIT_FAILURE);
	case SIGTERM:
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
	case SIGALRM:
		print_dump_data();

		increase_day();
		if(get_day() >= SO_DAYS) close_all("================================[END SIMULATION]================================\n", EXIT_SUCCESS);

		send_to_all_childs(SIGDAY);
		alarm(DAY_SEC);
		break;
	}
}

void send_to_all_childs(int signal){
	int i;
	for(i = 0; i < childs_counter; i++)
		SEND_SIGNAL(childs_pid, signal);
}

void close_all(const char *message, int exit_status)
{
	int i, pid;

	/* Messanges and exit */
	if (exit_status == EXIT_SUCCESS) dprintf(1, "\n%s\n", message);
	else dprintf(2, "\n%s\n", message);

	/* Killing and wait child */
	send_to_all_childs(SIGINT);
	while(wait(NULL) != -1 || errno == EINTR) errno = EXIT_SUCCESS;

	/* Closing semaphors */
	semctl(_id_sem, 0, IPC_RMID);
	semctl(_id_sem_docks, 0, IPC_RMID);
	semctl(_id_sem_cargo, 0, IPC_RMID);
	/* Closing message queues */
	msgctl(_id_msg_in_ports, IPC_RMID, NULL);
	msgctl(_id_msg_out_ports, IPC_RMID, NULL);

	close_shm_manager();

	/* Messanges and exit */
	dprintf(1, "[CLOSING OPERATION] Success");
	exit(exit_status);
}
