#include "User_Process.h"
#include "common.c"
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}
int macros[N_MACRO];
int main(int argc, char const *argv[])
{
    int id,key,i;
    struct shm_buf *buf;
    struct child *pid_users;
    /*shared memory key to access shared memory with macros*/
    key=atoi(argv[0]);

    pid_users=malloc(N_USERS*sizeof(struct child));

    id=shmget(key,sizeof(buf->mtext)+sizeof(struct child)*(N_USERS+N_NODES),IPC_CREAT);/*getting the ID of the shared memory*/

    buf=shmat(id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/
    printf("ID della SHM:%d\n",id);
    TEST_ERROR
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=buf->mtext[i];
    }


    TEST_ERROR
    printf("IM A CHILD BRO AND MY PID IS:%d\n",getpid());
    shmdt(buf);
    return 0;
}

struct transaction creaTransazione(pid_t rec, unsigned int budget){
    	struct transaction tr;
    if(budget <2){ 
        return tr;
    }else{/*if budget >=2 I proceed to create and send transaction*/
    struct timespec curr_time;
    int seed;
	int amount;
    int user;
    struct child arrayChildren[2];/*instead of 2 it should be N_USERS. c90 sucks*/

    seed=curr_time.tv_nsec;
    srand(seed);/*initializing RNG seed*/
    amount=rand()%(budget+1) +2;/*range [2,budget]. Cannot send more money than current budget*/
    clock_gettime(CLOCK_REALTIME,&curr_time);
    
    user=rand()%N_USERS;/*range [0,N_USERS-1]*/
	tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.sender = getpid();/*PID of the process creating the transaction*/
	tr.receiver = arrayChildren[user].pid;/*choosing a random user in the range [0,N_USERS-1]*/
	tr.reward = (int)(SO_REWARD * (float)amount);/*commission for node process*/
	tr.amount = amount - tr.reward;/*amount to be sent is equal to: tr.amount-(so_reward*amount)*/
    return tr;
    }
    
	
}

