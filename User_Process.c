#include "User_Process.h"
#include "common.c"
#include <math.h>
#define _GNU_SOURCE  /* Needed to compile with -std=c89 -pedantic flags*/
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}
/*Arrays declared globally so that I can use them inside functions*/
int macros[N_MACRO];
child *pid_users;
int main(int argc, char const *argv[])
{
    int id,i;
    
    transaction tr;
    int budget;
    
    id=atoi(argv[1]);/*shared memory id to access shared memory with macros*/
    shm_buf=(child*)shmat(id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    printf("[USER CHILD #%d] ID della SHM:%d\n",getpid(),id);
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_buf[i].pid;
    }
    /*Initializing budget*/
    budget=SO_BUDGET_INIT;
    /*Storing all users data in a local variable*/
    pid_users=malloc(sizeof(child)*N_USERS);
    for(i=N_MACRO;i<N_MACRO+N_USERS;i++){
        pid_users[i].pid=shm_buf[i].pid;
        pid_users[i].status=shm_buf[i].status;
    }
   /*Creating a new transaction*/
    tr=creaTransazione(budget);
    TEST_ERROR

    shmdt(shm_buf);
    printf("[USER CHILD] ABOUT TO ABORT\n");

    return 0;
}




/*
    Method that create a new transaction.
    If budget is less than 2, no transaction will be created. 
*/
struct transaction creaTransazione(unsigned int budget){
    transaction tr;
    if(budget <2){ 
        return tr;
    }else{
    struct timespec curr_time;
	int tmp_budget;
    int user;
    
    clock_gettime(CLOCK_REALTIME,&curr_time);
    srand(curr_time.tv_nsec);/*initializing RNG seed with the current clocktime(?)*/
    tmp_budget=rand()%(budget+1) +2;/*rSelecting random budget in range [2,budget]. Cannot send more money than current budget*/
    
    do{
        user=rand()%N_USERS;/*Selecting a random user in range [0,N_USERS-1]*/
    }while(pid_users[user].pid == getpid());/*This way I force sender not to send the transaction to itself*/
    

	tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.sender = getpid();/*PID of the process creating the transaction*/
	tr.receiver = pid_users[user].pid;/*choosing a random user in the range [0,N_USERS-1]. 12 is the offset(and number of macro) in the shm buffer*/
	tr.amount = tmp_budget - tr.reward;/*amount to be sent is equal to: tr.amount-(so_reward*amount)*/
    /*commission for node process*/
    tr.reward=(int)((double)SO_REWARD/100*tmp_budget);/* percentage/100 * budget ie: 12% of 500 -> 12/100 * 500*/
    return tr;
    }
    
	
}

