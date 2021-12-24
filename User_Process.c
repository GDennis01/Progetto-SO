#include "User_Process.h"
#include "common.c"
#include <math.h>
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
    int id,msgId,key,i;
    child *pid_users;
    transaction tr;
    int budget;
    message messaggio;
    
    id=atoi(argv[1]);/*shared memory id to access shared memory with macros*/
    msgId = atoi(argv[3]);
    shm_buf=(child*)shmat(id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    printf("ID della SHM:%d\n",id);

    
    
    
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_buf[i].pid;
    }
    budget=SO_BUDGET_INIT;
    for(i=0;i<N_MACRO;i++){
        /*printf("SONO FIGLIO PID %d -> Macro %d°:%d\n",getpid(),i+1,macros[i]);*/
    }
    for(i=0;i<N_USERS;i++){
        /*printf("PID %d°:%d\n",i+1,shm_buf[N_MACRO+i].pid);*/
    }
   printf("La percentuale reward è di %d percento \n",SO_REWARD);
    tr=creaTransazione(budget,N_USERS,SO_REWARD);
    printf("Struct transazione:\n\tTimestamp:%ld\n\tReceiver:%d\n\tSender:%d\n\tAmount:%d\n\tReward:%d\n",tr.timestamp,tr.receiver,tr.sender,tr.amount,tr.reward);
    TEST_ERROR
	
	/* ficca la transazione nella message queue */
	messaggio = creaMessaggio(tr);
	msgsnd ( msgId , messagio , sizeof(transaction), 0 ) /* la size non include il type */
	
    shmdt(shm_buf);
    return 0;
}

struct transaction creaTransazione(unsigned int budget,int n_users,int percentage_reward){
    transaction tr;
    if(budget <2){ 
        
        return tr;
    }else{/*if budget >=2 I proceed to create and send transaction*/
    struct timespec curr_time;
	int tmp_budget;
    int user;
    
    clock_gettime(CLOCK_REALTIME,&curr_time);
    srand(curr_time.tv_nsec);/*initializing RNG seed with the current timestamp(?)*/
    tmp_budget=rand()%(budget+1) +2;/*range [2,budget]. Cannot send more money than current budget*/
    
    

    do{
    user=rand()%n_users;/*range [0,N_USERS-1]*/
    }while(shm_buf[N_MACRO+user].pid == getpid());/*This way I force sender not to send the transaction to itself*/
    

	tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.sender = getpid();/*PID of the process creating the transaction*/
	tr.receiver = shm_buf[N_MACRO+user].pid;/*choosing a random user in the range [0,N_USERS-1]. 12 is the offset(and number of macro) in the shm buffer*/
	tr.amount = tmp_budget - tr.reward;/*amount to be sent is equal to: tr.amount-(so_reward*amount)*/
    /*commission for node process*/
    tr.reward=(int)((double)percentage_reward/100*tmp_budget);/* percentage/100 * budget ie: 12% -> 12/100 * 500*/
    return tr;
    }
    	
}

struct messagge creaMessaggio(struct transazione tr){
	messagge msg;
	msg.type = 1; /*a caso */
	msg.transaction = tr;
	return msg;
}

