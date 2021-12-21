#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}

struct shm_buf{
        int mtext[12];
		struct children_pid_array children[2];
};
struct children_pid_array{
	struct child* children;
};
struct transaction{
	time_t timestamp;
	pid_t sender;
	pid_t receiver;
	unsigned int amount;
	unsigned int reward;
};
struct child {
	pid_t pid;
	unsigned int status; // 0 se morto, 1 se vivo
};
/*Function used to read macros from "macros.txt". They are then saved in macros*/
void read_macros(int fd,int * macros);
struct transaction creaTransazione(pid_t rec, unsigned int budget,int N_USERS,int PERCENTUALE_REWARD);