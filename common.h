/*
	Ultime modifiche:
	-30/12/2021
		-Aggiunta del campo "executed" alla struct transaction
		-Aggiunta della struct "transaction_block"
		-Aggiunta della variabile legata al libro mastro
		-Aggiunta del prototipo della funzione "creaTransazione"
*/

#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <math.h>
#include <signal.h>
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}

#define N_MACRO 12 /*Numbers of macros to read from "macros.txt"*/
/*Macros*/
#define N_USERS macros[0]
#define N_NODES macros[1]
#define MIN_TRANS_GEN_NSEC macros[2]
#define MAX_TRANS_GEN_NSEC macros[3] 
#define MIN_TRANS_PROC_NSEC macros[4]
#define MAX_TRANS_PROC_NSEC macros[5]
#define SO_BUDGET_INIT macros[6]
#define SO_REWARD macros[7]
#define SO_RETRY macros[8]
#define SO_TP_SIZE macros[9]
#define SO_N_FRIENDS macros[10]
#define SO_SIM_SEC macros[11]
#define SO_BLOCK_SIZE 5
#define SO_REGISTRY_SIZE 7

/*Struct used to send/read data from shared memory*/
struct child *shm_buf;

/*Shared variable used to store macros*/
int *shm_macro;

/*Struct used to define a transaction*/
typedef struct transaction{
	unsigned int executed;/*whether the transaction has been processed(1) or not(0)*/
	time_t timestamp;
	pid_t sender;
	pid_t receiver;
	int amount;
	int reward;
} transaction;

/*Struct used to define a single transaction block to be then processed*/
typedef struct transaction_block{
	unsigned int executed;
	transaction transactions[SO_BLOCK_SIZE];
} transaction_block;

/*Variable used to communicate with shared memory and to log info on the ledger(Libro mastro)*/
 transaction_block *mastro_area_memoria;


/*Struct used to store info related to children processes(users and nodes)*/
typedef struct info_process{
	int budget;
	pid_t pid;
	/*TODO: flexata di collassare il tipo in abort_trans. 24 bit significativi per abort, i restanti per il tibo*/
	short int type;	/*0 for Users    1 for Nodes*/
	/*
	-type 0:  abort_trans=1 -> user aborted prematurely,  abort_trans=0 -> user aborted normally
	-type 1:  abort_trans=# of transaction* in trans pool
	-abort_trans=-1  process has been just created
	*/
	int abort_trans;
}info_process;

info_process *shm_info;

/*Buffer used to send/receive transactions by users and node*/
typedef struct msgqbuf{
	long mtype;
	transaction tr;
}msgqbuf;

/*Struct used to define a child(either a user or a node). Used also to store macros*/
typedef struct child {
	pid_t pid;
	unsigned int status; /* 0 se morto, 1 se vivo*/
} child;

/*Function used to read macros from "macros.txt". They are then saved in the then defined variable macros*/
void read_macros(int fd,int * macros);
/*Function used to create a new transaction*/
void initIPCS(int *info_key,int *macro_key,int *sem_key, int *mastro_key, int dims);
void deleteIPCs(int info_key,int macro_key,int sem_key, int mastro_key);
int getBudget(int my_index);
void updateBudget(int costoTransazione, int my_index);
void updateInfos(int budget,int abort_trans,int my_index);
void terminazione(int reason,int dim);
void signalsHandler(int sig);
int creaTransazione(struct transaction*,int budget);	