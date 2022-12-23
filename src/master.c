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

/* Macros */
#define DAY_SEC 1
#define CREATE_SHM(key, id, pointer, num_elem, extra_flags)\
		do{\
			(id) = shmget((key), sizeof(*(pointer))*(num_elem), 0600 | (extra_flags));\
			(pointer) = shmat(id, NULL, 0);\
		}while(0)
#define CLOSE_SHM(id, pointer) \
		do {\
			if(id >= 0){\
				shmctl((id), IPC_RMID, NULL);\
				if(pointer != NULL) detach((pointer));\
			} \
		}while (0)


/* Global variables */
int _id_data;
int _id_sem;
int _weather_pid = 0;
/* Shared memory */
struct general *_data;
struct port *_data_port;
struct ship *_data_ship;
struct cargo *_data_cargo;
struct supply_demand *_data_supply_demand;

/* Prototypes */
void initialize_shared();
pid_t create_proc(char *, int);
void custom_handler(int);
void close_all(const char *, int);
void create_children();
void read_constants_from_file();
void print_dump_data();
void loop();

int main()
{
	struct sigaction sa;
	sigset_t set_masked;

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
	/* sigprocmask(SIG_BLOCK, &set_masked, NULL); */
	initialize_shared();
	/* sigprocmask(SIG_UNBLOCK, &set_masked, NULL); */

	/* Create and start children*/
	semctl(_id_sem, 0, SETVAL, 1);
	create_children();
	execute_single_sem_oper(_id_sem, 0, -1);

	/* Wait forever */
	alarm(DAY_SEC);
	while (1) pause();
}

void initialize_shared()
{
	/* SHM: General */
	CREATE_SHM(KEY_SHARED, _id_data, _data, 1, IPC_CREAT | IPC_EXCL);
	read_constants_from_file();
	CREATE_SHM(IPC_PRIVATE, _data->id_ship, _data_ship, _data->SO_NAVI, 0);
	CREATE_SHM(IPC_PRIVATE, _data->id_port, _data_port, _data->SO_PORTI, 0);
	CREATE_SHM(IPC_PRIVATE, _data->id_cargo, _data_cargo, _data->SO_MERCI, 0);
	CREATE_SHM(IPC_PRIVATE, _data->id_supply_demand, _data_supply_demand,
			_data->SO_MERCI * _data->SO_PORTI, 0);

	/* MSG port in and out */
	_data->id_msg_in_ports = msgget(IPC_PRIVATE, 0600);
	_data->id_msg_out_ports = msgget(IPC_PRIVATE, 0600);

	/* SEM: Start and docks */
	_id_sem = semget(KEY_SEM, 1, 0600 | IPC_CREAT | IPC_EXCL);
	_data->id_sem_docks = semget(IPC_PRIVATE, _data->SO_PORTI, 0600);
	_data->id_sem_cargo = semget(IPC_PRIVATE, _data->SO_MERCI, 0600);

}

void print_dump_data()
{
	int port, type, ship, cargo_type, cargo_in_port, quantity;
	int tot_port_swell = 0;
	int tot_ship_storm = 0, tot_ship_maelstrom = 0, tot_ship_dock = 0, tot_ship_empty = 0, tot_ship_cargo = 0;
	int tot_cargo_port = 0, tot_cargo_ship = 0, tot_cargo_del = 0, tot_cargo_exp_ship = 0, tot_cargo_exp_port = 0;

	dprintf(1, "\n\n================================[DAY %3d]=================================\n", _data->today);

	/* Dumps things */
	dprintf(1, "\n================================[DUMPS]===================================\n");

	dprintf(1, "[PORTS]\n");
	for (port = 0; port < _data->SO_PORTI; port++){
		cargo_in_port = 0;
		dprintf(1, "|----(Port: %d) tot_docks: %d, used_docks: %d, swell: %d. Cargo:\n", port, _data_port[port].dump_dock_tot,
				semctl(_data->id_sem_docks, port, GETVAL), _data_port[port].dump_had_swell);
		tot_port_swell += _data_port[port].dump_had_swell;

		for (type = 0; type < _data->SO_MERCI; type++){
			quantity = _data_supply_demand[_data->SO_PORTI * port + type].quantity;
			cargo_in_port = quantity > 0 ? quantity : 0;
			dprintf(1, "|    |----(Cargo type: %d) in_port: %d, sent: %d, received: %d\n", type, cargo_in_port,
					_data_supply_demand[_data->SO_PORTI * port + type].dump_tot_sent,
					_data_supply_demand[_data->SO_PORTI * port + type].dump_tot_received);
		}
		dprintf(1, "|\n");
	}
	dprintf(1, "|--PORTS TOTALS: tot_swell: %d\n\n", tot_port_swell);

	dprintf(1, "[SHIPS]\n");
	for (ship = 0; ship < _data->SO_NAVI; ship++){
		dprintf(1, "|----(Ship: %d) is_at_dock: %d, had_storm: %d, had_maeltrom: %d\n", ship, _data_ship[ship].dump_is_at_dock,
				_data_ship[ship].dump_had_storm, _data_ship[ship].dump_had_maelstrom);
		if (_data_ship[ship].dump_is_at_dock) tot_ship_dock++;
		else if (_data_ship[ship].capacity == _data->SO_CAPACITY) tot_ship_empty++;
		else tot_ship_cargo++;
		tot_ship_storm += _data_ship[ship].dump_had_storm;
		tot_ship_maelstrom += _data_ship[ship].dump_had_maelstrom;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--SHIPS TOTALS: tot_at_dock: %d, tot_cargo: %d, tot_empty: %d, tot_storm: %d, tot_maeltrom: %d\n\n",
			tot_ship_dock, tot_ship_cargo, tot_ship_empty, tot_ship_storm, tot_ship_maelstrom);

	dprintf(1, "[CARGO]\n");
	for (cargo_type = 0; cargo_type < _data->SO_MERCI; cargo_type++){
		dprintf(1, "|----(Cargo type: %d) tot_in_ports: %d, tot_in_ships: %d, tot_delivered: %d, tot_expired_port: %d, tot_expired_ship: %d\n",
				cargo_type, _data_cargo[cargo_type].dump_at_port, _data_cargo[cargo_type].dump_in_ship, _data_cargo[cargo_type].dump_tot_delivered,
				_data_cargo[cargo_type].dump_exipered_port, _data_cargo[cargo_type].dump_exipered_ship);

				/* Totals */
				tot_cargo_port += _data_cargo[cargo_type].dump_at_port;
				tot_cargo_ship += _data_cargo[cargo_type].dump_in_ship;
				tot_cargo_del += _data_cargo[cargo_type].dump_tot_delivered;
				tot_cargo_exp_port += _data_cargo[cargo_type].dump_exipered_port;
				tot_cargo_exp_ship += _data_cargo[cargo_type].dump_exipered_ship;
	}
	dprintf(1, "|\n");
	dprintf(1, "|--CARGO TOTALS: tot_cargo_port: %d, tot_cargo_ship: %d, tot_cargo_delivered: %d, tot_cargo_expired_port: %d, tot_cargo_expired_ship: %d\n\n",
			tot_cargo_port, tot_cargo_ship, tot_cargo_del, tot_cargo_exp_port, tot_cargo_exp_ship);

	/* Shop things */
	dprintf(1, "================================[SHOP]====================================\n");
	for (port = 0; port < _data->SO_PORTI; port++){
		dprintf(1, "PORT %d:", port);
		for (type = 0; type < _data->SO_MERCI; type++){
			dprintf(1, "(t: %d) %d:", type,
					_data_supply_demand[_data->SO_PORTI * port + type].quantity);
		}
		dprintf(1, "\n");
	}
	dprintf(1, "\n================================[END SHOP]================================\n\n\n");

	if(_data->today >= _data->SO_DAYS)
		close_all("================================[END SIMULATION]================================\n", EXIT_SUCCESS);
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

	/* TODO BZERO TO EVERYTHING UP + SECURITY */
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
			current_port->x = RANDOM_DOUBLE(0, SO_LATO);
			current_port->y = RANDOM_DOUBLE(0, SO_LATO);
		}

		n_docks = RANDOM(1, SO_BANCHINE);
		current_port->dump_dock_tot = n_docks;
		semctl(_data->id_sem_docks, i, SETVAL, n_docks);

		current_port->dump_had_swell = FALSE;
	}

	/* Initialize ships data */
	for (i = 0; i < SO_NAVI; i++){
		current_ship = &_data_ship[i];
		current_ship->pid = create_proc("./ship", i);

		current_ship->x = RANDOM_DOUBLE(0, SO_LATO);
		current_ship->y = RANDOM_DOUBLE(0, SO_LATO);
		current_ship->is_moving = FALSE;

		/* Initializing ship dump */
		current_ship->capacity = _data->SO_CAPACITY;
		current_ship->dump_is_at_dock = FALSE;
		current_ship->dump_had_storm = FALSE;
		current_ship->dump_had_maelstrom = FALSE;
	}

	/* Initialize cargo data */
	for (i = 0; i < SO_MERCI; i++){
		current_cargo = &_data_cargo[i];

		current_cargo->weight_batch = RANDOM(1, SO_SIZE);
		current_cargo->shelf_life = RANDOM(SO_MIN_VITA, SO_MAX_VITA);

		/* Initializing cargo bump */
		current_cargo->dump_at_port = 0;
		current_cargo->dump_in_ship = 0;
		current_cargo->dump_exipered_port = 0;
		current_cargo->dump_exipered_ship = 0;
		current_cargo->dump_tot_delivered = 0;

		/* Semaphore */
		semctl(_data->id_sem_cargo, i, SETVAL, 1);
	}

	/* Initialize supply and demand */
	bzero(_data_supply_demand, sizeof(*_data_supply_demand) * _data->SO_PORTI * _data->SO_MERCI);

	/* Initialize weather */
	_weather_pid = create_proc("./weather", -1);
}

void read_constants_from_file() /* Crashable */
{
	const NUM_VALUE = 16;
	int num_read, counter, ival;
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
				_data->SO_LATO = value;
				dprintf(1, " %lf", value);
			}else {
				ival = (int) value;
				((int *)(&_data->SO_DAYS))[counter - 1] = ival;
				dprintf(1, " %d", ival);
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
		_data->today++;
		print_dump_data();

		for (i = 0; i < _data->SO_PORTI; i++)
			SEND_SIGNAL(_data_port[i].pid, SIGDAY);
		for (i = 0; i < _data->SO_NAVI; i++)
			SEND_SIGNAL(_data_ship[i].pid, SIGDAY);
		SEND_SIGNAL(_weather_pid, SIGDAY);

		alarm(DAY_SEC);
		break;
	}
}

void close_all(const char *message, int exit_status)
{
	int i, pid;

	/* Messanges and exit */
	if (exit_status == EXIT_SUCCESS) dprintf(1, "\n%s\n", message);
	else dprintf(2, "\n%s\n", message);

	/* Killing and wait child */
	SEND_SIGNAL(_weather_pid, SIGINT);
	for (i = 0; _data_port != NULL && i<_data->SO_PORTI; i++)
		SEND_SIGNAL(_data_port[i].pid, SIGINT);
	for (i = 0; _data_ship != NULL && i<_data->SO_NAVI; i++)
		SEND_SIGNAL(_data_ship[i].pid, SIGINT);
	while(wait(NULL) != -1 || errno == EINTR) errno = EXIT_SUCCESS;

	/* Closing semaphors */
	semctl(_id_sem, 0, IPC_RMID);
	semctl(_data->id_sem_docks, 0, IPC_RMID);
	semctl(_data->id_sem_cargo, 0, IPC_RMID);
	/* Closing message queues */
	msgctl(_data->id_msg_in_ports, IPC_RMID, NULL);
	msgctl(_data->id_msg_out_ports, IPC_RMID, NULL);
	/* Closing shared memories */
	CLOSE_SHM(_data->id_port, _data_port);
	CLOSE_SHM(_data->id_ship, _data_ship);
	CLOSE_SHM(_data->id_cargo, _data_cargo);
	CLOSE_SHM(_data->id_supply_demand, _data_supply_demand);
	CLOSE_SHM(_id_data, _data); /* must be last */

	/* Messanges and exit */
	dprintf(1, "[CLOSING OPERATION] Success");
	exit(exit_status);
}
