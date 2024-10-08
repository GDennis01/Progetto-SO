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

/*Struct used to send/read data from shared memory*/
struct child *shm_buf;

typedef struct transaction{
	time_t timestamp;
	pid_t sender;
	pid_t receiver;
	 int amount;
	 int reward;
} transaction;

/*Struct used to define a child(either a user or a node). Used also to store macros*/
typedef struct child {
	pid_t pid;
	unsigned int status; /* 0 se morto, 1 se vivo*/
} child;

/*Function used to read macros from "macros.txt". They are then saved in the then defined variable macros*/
void read_macros(int fd,int * macros);
/*Function used to create a new transaction*/
struct transaction creaTransazione( unsigned int budget);