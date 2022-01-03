/*
    Ultime modifiche
     -03/01/2021
        -Ora ci sono solo tre key: info,macro e sem
        -Le macro vora engono salvate in *shm_macro, le info dei processi in *info
        -alarmHandler -> signalsHandler
        -Aggiunto mastro_key/id e la sua relativa shared memory

    -30/12/2021
        -Aggiunta "tr.executed=1" alla fine di creazione di una transazione
*/
#include "User_Process.h"

 /* Needed to compile with -std=c89 -pedantic flags*/
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}
/*Arrays declared globally so that I can use them inside functions*/
int macros[N_MACRO];
info_process *pid_users;/*Variable used to store pid of users locally*/
info_process *pid_nodes;/*Variable used to store pid of nodes locally*/
int main(int argc, char const *argv[])
{
    struct sigaction sa;
    /*
    info_id: id of shm_info
    macro_id: id of shm_macro
    sem_id:id of the semaphore  
    mastro_id: id of ledger(libro mastro)
    msgq_id: id of the message queue of the selected node
    */
    int info_id,macro_id,sem_id,mastro_id,msgq_id;
    int i,j,budget;
    pid_t pid;
    transaction tr;/*transction to be sent*/
    transaction * tr_block,*tr_pool;
    msgqbuf msg_buf;/*Buffer used to send transaction*/
    struct sembuf sops;/*variable used to perform actions on semaphores*/
    info_process infos;/*variable used to store data of the current process locally*/

    /*Initializing infos that will be sent every second to the master process*/
    infos.pid=getpid();
    infos.type=0;
    infos.abort_trans=0;
    
    
    info_id=atoi(argv[1]);/*shared memory id to access shared memory with info related to processes*/
    macro_id=atoi(argv[2]);/*shared memory id to access shared memory with macros*/
    sem_id=atoi(argv[3]);/*id of semaphore*/
    mastro_id=atoi(argv[4]);
    
    
    shm_macro=shmat(macro_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/
    shm_info=shmat(info_id,NULL,SHM_RDONLY);/*Attaching to shm with info related to processes*/
    /*msgq_id=atoi(argv[2]);*/
    /*printf("[USER CHILD #%d] ID del SEM:%d\n",getpid(),sem_id);*/

    /*The semaphore is used to wait for nodes to finish creating their queues*/
    sops.sem_num=1;
    sops.sem_op=0;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);

    printf("WAITING FOR SEM\n");
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_macro[i];
    }

    /*Initializing budget*/
    budget=SO_BUDGET_INIT;
    infos.budget=budget;

    /*Populating the pid user array*/
    pid_users=malloc(sizeof(info_process)*N_USERS);
    for(i=0;i<N_USERS;i++){
        pid_users[i].pid=shm_info[i].pid;  
        pid_users[i].type=0;
        /*printf("[USER CHILD #%d] Leggo User #%d \n",getpid(),pid_users[i].pid);  */
    }

    /*Populating the pid node array*/
    pid_nodes=malloc(sizeof(info_process)*N_NODES);
    for(i=0;i<N_NODES;i++){
        pid_nodes[i].pid=shm_info[i+N_USERS].pid;  
        pid_users[i].type=1;
        /*printf("[USER CHILD #%d] Leggo Nodo #%d \n",getpid(),pid_nodes[i].pid);  */
    }

    
    
    /*Creating a new transaction*/
    tr = creaTransazione(budget);
    /*printTransaction(tr);*/
    /*Sending the transaction to a random selected node.*/
    pid=getRndNode();
    msg_buf.mtype=pid_nodes[pid].pid;/*That way, only the selected node can read the message with type set to its pid*/
    msg_buf.tr=tr;

    msgq_id=msgget(pid_nodes[pid].pid,0660);
    TEST_ERROR
    msgsnd(msgq_id,&msg_buf,sizeof(msg_buf.tr),0);

    printf("[USER CHILD #%d] INVIO A MSGQ_ID: %d  -> NODO SELEZIONATO:%d\n",getpid(),msgq_id,pid_nodes[pid].pid);
    
    shmdt(shm_buf);
    printf("[USER CHILD #%d] ABOUT TO ABORT\n",getpid());

    
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
        user=(rand()%N_USERS);/*Selecting a random user in range [0,N_USERS-1]*/
    }while(pid_users[user].pid == getpid());/*This way I force sender not to send the transaction to itself*/
    

	tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.sender = getpid();/*PID of the process creating the transaction*/
	tr.receiver = pid_users[user].pid;/*choosing a random user in the range [0,N_USERS-1]. 12 is the offset(and number of macro) in the shm buffer*/
	tr.amount = tmp_budget - tr.reward;/*amount to be sent is equal to: tr.amount-(so_reward*amount)*/
    /*commission for node process*/
    tr.reward=(int)((double)SO_REWARD/100*tmp_budget);/* percentage/100 * budget ie: 12% of 500 -> 12/100 * 500*/
    tr.executed=1;
    return tr;
    }
}
void printTransaction(struct transaction tr){
    printf("[USER CHILD #%d]Sender:%d\n",getpid(),tr.sender);
    printf("[USER CHILD #%d]Receiver:%d\n",getpid(),tr.receiver);
    printf("[USER CHILD #%d]Reward:%d\n",getpid(),tr.reward);
    printf("[USER CHILD #%d]Timestamp:%ld\n",getpid(),tr.timestamp);
    printf("[USER CHILD #%d]Amount:%d\n",getpid(),tr.amount);
}
int getRndNode(){
	return rand()%N_NODES;
}

void updateInfos(int budget,int abort_trans,info_process* infos){
    infos->pid=getpid();
    infos->type=0;
    infos->budget=budget;
    infos->abort_trans=abort_trans;
}
/*TODO: fixare handle_signal(errori sintattici etc)*/
void signalsHandler(int signal) {

    /*updateInfos(budget, 0, infos); */
    exit(EXIT_SUCCESS);
}

