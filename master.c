#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include "shared_mem.h"
#include "semaphore.h"
#include "utils.h"

/* Macros */
#define CLOSE_SHM(id, pointer) \
		do {\
			shmctl((id), IPC_RMID, NULL);\
			detach((pointer));\
		}while (0)

/* Global variables */
int _id_data;
int _id_sem;
/* Shared memory */
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;
struct cargo *_data_cargo;
int *_data_supply_demand;

/* Prototypes */
void initialize_shared();
pid_t create_proc(char *, int);
void custom_handler(int);
void close_all(const char *, int);
void create_children();
void read_constants_from_file();
void create_shared_structures();
double get_random_coord();
void loop();

int main()
{
	struct sigaction sa;
	sigset_t set_masked;

	/* Initializing */
	srand(time(NULL));
	initialize_shared();

	/* Create and start children*/
	create_children();
	execute_single_sem_oper(_id_sem, 0, -1);

	/* Setting signal handler (need to be done after) */
	bzero(&sa, sizeof(sa));
	sa.sa_handler = &custom_handler;
	sigfillset(&set_masked);
	sa.sa_mask = set_masked;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);

	/* Start running*/
	loop();
}

void initialize_shared()
{
	int id;

	/* SHM: General */
	id = shmget(KEY_SHARED, sizeof(*_data), 0600 | IPC_CREAT | IPC_EXCL);
	_data = shmat(id, NULL, 0);
	_id_data = id;
	read_constants_from_file();

	/* SHM: Port */
	id = shmget(IPC_PRIVATE, sizeof(*_data_port) * _data->SO_PORTI, 0600);
	_data_port = shmat(id, NULL, 0);
	_data->id_port = id;

	/* SHM: Suppy and demand of ports*/
	id = shmget(IPC_PRIVATE, sizeof(*_data_supply_demand) * _data->SO_PORTI * _data->SO_MERCI, 0600);
	_data_supply_demand = shmat(id, NULL, 0);
	_data->id_supply_demand = id;

	/* SHM: Ship */
	id = shmget(IPC_PRIVATE, sizeof(*_data_ship) * _data->SO_NAVI, 0600);
	_data_ship = shmat(id, NULL, 0);
	_data->id_ship = id;

	/* SHM: Cargo */
	id = shmget(IPC_PRIVATE, sizeof(*_data_cargo) * _data->SO_MERCI, 0600);
	_data_cargo = shmat(id, NULL, 0);
	_data->id_cargo = id;

	/* MSG: ports in */
	id = msgget(IPC_PRIVATE, 0600);
	_data->id_msg_in_ports = id;

	/* MSG: ports out */
	id = msgget(IPC_PRIVATE, 0600);
	_data->id_msg_out_ports = id;

	/* SEM: Start */
	id = semget(KEY_SEM, 1, 0600 | IPC_CREAT | IPC_EXCL);
	semctl(id, 0, SETVAL, 1);
	_id_sem = id;

	/* SEM: Docks */
	id = semget(IPC_PRIVATE, _data->SO_PORTI, 0600);
	_data->id_sem_docks = id;
}

void loop()
{
	while (1){
		dprintf(1, "[Parent] Wait\n");
		pause(); /* TODO get data */
	}
}

void create_children()
{
	/* Constants for readability */
	const SO_DAYS = _data->SO_DAYS;
	const SO_PORTI =  _data->SO_PORTI;
	const SO_FILL = _data->SO_FILL;
	const SO_LATO = _data->SO_LATO;
	const SO_NAVI = _data->SO_NAVI;
	const SO_BANCHINE = _data->SO_BANCHINE;
	const SO_MERCI = _data->SO_MERCI;
	const SO_SIZE = _data->SO_SIZE;
	const SO_MIN_VITA = _data->SO_MIN_VITA;
	const SO_MAX_VITA = _data->SO_MAX_VITA;

	int i, to_add, daily, n_docks;
	struct port *current_port;
	struct ship *current_ship;
	struct cargo *current_cargo;

	/* Initialize ports data */
	daily = SO_FILL / (SO_DAYS * SO_PORTI);
	for (i = 0; i < SO_PORTI; i++){
		current_port = &_data_port[i];
		current_port->pid = create_proc("./port", i);

		to_add = (i < (SO_FILL % (SO_DAYS * SO_PORTI))) ? 1 : 0;
		current_port->daily_restock_capacity = daily + to_add;

		if (i<4){
			/* ports in 4 corner */
			current_port->x = i % 2 != 0 ? SO_LATO : 0;
			current_port->y = i < 2 ? SO_LATO : 0;
		}
		else{
			current_port->x = get_random_coord();
			current_port->y = get_random_coord();
		}

		n_docks = get_random(1, SO_BANCHINE);
		semctl(_data->id_sem_docks, i, SETVAL, n_docks);
	}

	/* Initialize ships data */
	for (i = 0; i < SO_NAVI; i++){
		current_ship = &_data_ship[i];
		current_ship->pid = create_proc("./ship", i);

		current_ship->x = get_random_coord();
		current_ship->y = get_random_coord();
		current_ship->is_moving = FALSE;
	}

	/* Initialize cargo data */
	for (i = 0; i < SO_MERCI; i++){
		current_cargo = &_data_cargo[i];

		current_cargo->weight_batch = get_random(1, SO_SIZE);
		current_cargo->shelf_life = get_random(SO_MIN_VITA, SO_MAX_VITA);
	}

	/* Initialize supply and demand */
	bzero(_data_supply_demand, sizeof(*_data_supply_demand) * _data->SO_PORTI * _data->SO_MERCI);
}

double get_random_coord()
{
	return rand() / (double)INT_MAX * _data->SO_LATO;
}

void read_constants_from_file()
{
	FILE *file;
	char name[100];
	int char_read, num, c;
	bool_t equal_found, num_found;
	file = fopen("constants.txt", "r");

	/*Reading*/
	num = 0;
	c = 0;
	name[0] = '\0';
	equal_found = FALSE;
	num_found = FALSE;
	while ((char_read = fgetc(file)) != EOF){
		if ((char_read >= 'A' && char_read <= 'Z') || char_read == '_'){
			if (equal_found)
				close_all("[Error] Couldn't parse constants.txt (found = followed by char)", EXIT_FAILURE);
			else if (c >= 99)
				close_all("[Error] Couldn't parse constants.txt (found name too big)", EXIT_FAILURE);
			name[c] = (char) char_read;
			name[++c] = '\0';
		}
		else if (char_read == '='){
			equal_found = TRUE;
		}
		else if (char_read >= '0' && char_read <= '9'){
			if (!equal_found)
				close_all("[Error] Couldn't parse constants.txt (not found = before number)", EXIT_FAILURE);

			num = num * 10 + (char_read - '0');
			num_found = TRUE;
			if(num < 0)
				close_all("[Error] Couldn't parse constants.txt (found number so big that exceed int)", EXIT_FAILURE);
		}
		else if (char_read == ';'){
			if(!num_found)
				close_all("[Error] Couldn't parse constants.txt (not found number before ;)", EXIT_FAILURE);
			/* Check */
			if (strcmp(name, "SO_LATO") == 0){
				_data->SO_LATO = num;
			}else if (strcmp(name, "SO_DAYS") == 0){
				_data->SO_DAYS = num;
			}else if (strcmp(name, "SO_NAVI") == 0){
				_data->SO_NAVI = num;
			}else if (strcmp(name, "SO_PORTI") == 0){
				_data->SO_PORTI = num;
			}else if (strcmp(name, "SO_MERCI") == 0){
				_data->SO_MERCI = num;
			}else if (strcmp(name, "SO_STORM_DURATION") == 0){
				_data->SO_STORM_DURATION = num;
			}else if (strcmp(name, "SO_SWELL_DURATION") == 0){
				_data->SO_SWELL_DURATION = num;
			}else if (strcmp(name, "SO_MAELSTORM") == 0){
				_data->SO_MAELSTORM = num;
			}else if (strcmp(name, "SO_FILL") == 0){
				_data->SO_FILL = num;
			}else if (strcmp(name, "SO_BANCHINE") == 0){
				_data->SO_BANCHINE = num;
			}else if (strcmp(name, "SO_LOADSPEED") == 0){
				_data->SO_LOADSPEED = num;
			}else if (strcmp(name, "SO_SIZE") == 0){
				_data->SO_SIZE = num;
			}else if (strcmp(name, "SO_SPEED") == 0){
				_data->SO_SPEED = num;
			}else if (strcmp(name, "SO_CAPACITY") == 0){
				_data->SO_CAPACITY = num;
			}else if (strcmp(name, "SO_MIN_VITA") == 0){
				_data->SO_MIN_VITA = num;
			}else if (strcmp(name, "SO_MAX_VITA") == 0){
				_data->SO_MAX_VITA = num;
			}else{
				close_all("[Error] Couldn't parse constants.txt (found invalid var name)", EXIT_FAILURE);
			}
			dprintf(1, "Set %d to %s\n", num, name);
			/* Reset */
			num = 0;
			c = 0;
			name[0] = '\0';
			equal_found = FALSE;
			num_found = FALSE;
		}
		else if (char_read == '/'){
			char_read = fgetc(file);
			if(char_read != '/')
				close_all("[Error] Couldn't parse constants.txt (found single /)", EXIT_FAILURE);

			while((char_read = fgetc(file)) != EOF && (char_read = fgetc(file)) != '\n'){}
		}
		else if(char_read != ' ' && char_read != '\n' && char_read != '\r' && char_read != '\t')
			close_all("[Error] Couldn't parse constants.txt (found invalid char)", EXIT_FAILURE);
	}

	while (0){
		fscanf(file, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			/* Generic simulation specifications */
			&_data->SO_LATO, &_data->SO_DAYS,
			&_data->SO_NAVI, &_data->SO_PORTI, &_data->SO_MERCI,
			/* Weather events max duration */
			&_data->SO_STORM_DURATION, &_data->SO_SWELL_DURATION, &_data->SO_MAELSTORM,
			/* Ports specifications */
			&_data->SO_FILL, &_data->SO_BANCHINE, &_data->SO_LOADSPEED,
			/* Ships specifications */
			&_data->SO_SIZE, &_data->SO_SPEED, &_data->SO_CAPACITY,
			/* Cargo specifications */
			&_data->SO_MIN_VITA, &_data->SO_MAX_VITA);
	}

	dprintf(1, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
			/* Generic simulation specifications */
			_data->SO_LATO, _data->SO_DAYS,
			_data->SO_NAVI, _data->SO_PORTI, _data->SO_MERCI,
			/* Weather events max duration */
			_data->SO_STORM_DURATION, _data->SO_SWELL_DURATION, _data->SO_MAELSTORM,
			/* Ports specifications */
			_data->SO_FILL, _data->SO_BANCHINE, _data->SO_LOADSPEED,
			/* Ships specifications */
			_data->SO_SIZE, _data->SO_SPEED, _data->SO_CAPACITY,
			/* Cargo specifications */
			_data->SO_MIN_VITA, _data->SO_MAX_VITA);
	close_all("TEST", 0);
	fclose(file);
}

pid_t create_proc(char *name, int index)
{
	pid_t proc_pid;
	char *arg[3], *env[]={NULL}, buf[10];

	if ((proc_pid = fork()) == -1){
		close_all("[FATAL] Couldn't fork", EXIT_FAILURE);
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

void custom_handler(int signal)
{
	switch (signal){
	case SIGTERM:
	case SIGINT:
		close_all("[INFO] Interruped by user", EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	int i;

	/* Killing and wait child */
	for (i = 0; i<_data->SO_PORTI; i++){
		if(_data_port[i].pid > 0)
			kill(_data_port[i].pid, SIGTERM);
	}
	for (i = 0; i < _data->SO_NAVI; i++){
		if(_data_ship[i].pid > 0)
			kill(_data_ship[i].pid, SIGTERM);
	}

	while(wait(NULL) != -1 || errno == EINTR);

	/* Removing semaphors */
	semctl(_id_sem, 0, IPC_RMID);
	semctl(_data->id_sem_docks, 0, IPC_RMID);

	/* Closing message queue (need to be done before detaching shm) */
	msgctl(_data->id_msg_in_ports, IPC_RMID, NULL);
	msgctl(_data->id_msg_out_ports, IPC_RMID, NULL);

	/* Detach and mark for removal IPC structures (needed in this order) */
	CLOSE_SHM(_data->id_port, _data_port);
	CLOSE_SHM(_data->id_ship, _data_ship);
	CLOSE_SHM(_data->id_cargo, _data_cargo);
	CLOSE_SHM(_data->id_supply_demand, _data_supply_demand);
	CLOSE_SHM(_id_data, _data); /* must be last */

	/* Messanges and exit */
	if (exit_status == EXIT_SUCCESS)
		dprintf(1, "\n%s\n", message);
	else
		dprintf(2, "\n%s\n", message);
	exit(exit_status);
}
