/*

    TODO: implementare la meccanica del SO_RETRY
    Ultime modifiche
     -06/01/2022
        -Parziale implementazione della lista di transazioni locali(cache)
        -Parziale implementazione del metodo checkLedger(tr);
        -Aggiunta di una fprintf per stampare la cache in un file (cache.txt) per il debug

     -05/01/2022
        -Fixato l'errore "Invalid Argument". 
         Risiedeva nella generazione di un tempo randomico. A volte andava in overflow e quindi la nanosleep dava errore
        -Aggiornato il metodo "creaTransazione()", ora restituisce un intero(0 se è andato a buon fine, 1 altrimenti)

     -04/01/2022
        -Modificato il metodo "updateInfos()"
        -Aggiunto il metodo "getBudget()"
        -Cambiata la gestione di invio delle proprie info, ora si usa solo la shm
        
     
     -03/01/2022
        -Ora ci sono solo tre key: info,macro e sem
        -Le macro vora engono salvate in *shm_macro, le info dei processi in *info
        -alarmHandler -> signalsHandler
        -Aggiunto mastro_key/id e la sua relativa shared memory
        -Implementato il ciclo(while(1))
        

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
FILE *fd;/*For debug purposes only(used to access the cache.txt)*/
struct sembuf sops;/*variable used to perform actions on semaphores*/
int sem_id;
int my_index=0;
int retry=0;
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
    int info_id,macro_id,mastro_id,msgq_id;
    int i,j,budget;
    

    pid_t pid;
    transaction tr;/*transction to be sent*/
    transaction * tr_block,*tr_pool;
    msgqbuf msg_buf;/*Buffer used to send transaction*/
    
    info_process infos;/*variable used to store data of the current process locally*/
    struct timespec time;

    fd=fopen("cache.txt","a");

    srand(getpid());
    
    /* signal handler */
    bzero(&sa,sizeof(sa));

    sa.sa_handler=signalsHandler;
    sigaction(SIGTERM,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);
    
    /*retrieving keys sent by master process*/
    info_id=atoi(argv[1]);/*shared memory id to access shared memory with info related to processes*/
    macro_id=atoi(argv[2]);/*shared memory id to access shared memory with macros*/
    sem_id=atoi(argv[3]);/*id of semaphore*/
    mastro_id=atoi(argv[4]);
    
    /*Attaching to shared memories*/
    shm_macro=(int*)shmat(macro_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/
    shm_info=(info_process*)shmat(info_id,NULL,0666);/*Attaching to shm with info related to processes*/
    mastro_area_memoria=(transaction_block*)shmat(mastro_id,NULL,0666);
    /*msgq_id=atoi(argv[2]);*/
    /*printf("[USER CHILD #%d] ID del SEM:%d\n",getpid(),sem_id);*/

   

    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_macro[i];
    }

    /*Populating the pid user array*/
    pid_users=malloc(sizeof(info_process)*N_USERS);
    for(i=0;i<N_USERS;i++){
        pid_users[i].pid=shm_info[i].pid;  
        pid_users[i].type=0;
        if(shm_info[i].pid==getpid()){
            my_index=i;
        }
        /*printf("[USER CHILD #%d] Leggo User #%d \n",getpid(),pid_users[i].pid);  */
    }

    /*Populating the pid node array*/
    pid_nodes=malloc(sizeof(info_process)*N_NODES);
    for(i=0;i<N_NODES;i++){
        pid_nodes[i].pid=shm_info[i+N_USERS].pid;  
        pid_users[i].type=1;
        /*printf("[USER CHILD #%d] Leggo Nodo #%d \n",getpid(),pid_nodes[i].pid);  */
    }

    updateInfos(SO_BUDGET_INIT, 0, my_index);

     /*The semaphore is used to wait for nodes to finish creating their queues*/
    sops.sem_num=1;
    sops.sem_op=0;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);
    /*Initializing the list of transaction sent but not yet written in the ledger*/
    trans_sent=(transaction*)malloc(sizeof(transaction));

    TEST_ERROR
    while(1){
        
        /*time.tv_nsec=(rand()+MIN_TRANS_GEN_NSEC)%(MAX_TRANS_GEN_NSEC+1); THIS METHOD RESULTED SOMETIMES IN OVERFLOW*/
        time.tv_nsec=rand()%(MAX_TRANS_GEN_NSEC+1-MIN_TRANS_GEN_NSEC) +MIN_TRANS_GEN_NSEC;/*[MIN_TRANS_GEN,MAX_TRANS_GEN]*/
        time.tv_sec=1;/*1 for debug mode xd*/
        
        /*Creating a new transaction*/
        /*tr = creaTransazione(getBudget(my_index));*/
        if(creaTransazione(&tr,getBudget(my_index)) == -1){
            printf("NOT ENOUGH BUDGET TO SEND A TRANSACTION\n");
            raise(SIGTERM);
        }else{
        
        /*printTransaction(tr);*/
        /*Sending the transaction to a random selected node.*/
        pid=getRndNode();
        msg_buf.mtype=pid_nodes[pid].pid;/*That way, only the selected node can read the message with type set to its pid*/
        msg_buf.tr=tr;
        
        msgq_id=msgget(pid_nodes[pid].pid,0666);
        if(msgsnd(msgq_id,&msg_buf,sizeof(msg_buf.tr),0) == -1){
            printf("[USER CHILD #%d] Errore. Transazione scartata\n",getpid());
            retry++;
        }else
            retry=0;
        if(retry == SO_RETRY){
            raise(SIGTERM);
        }

        /*Updating the local transaction list*/
        trans_sent[trans_sent_Index]= tr;/*Saving the transaction sent locally*/
        trans_sent_Index=trans_sent_Index+1;/*Incrementing by 1 the index*/
        trans_sent=realloc(trans_sent,sizeof(transaction)*(trans_sent_Index+1));/*Incrementing by 1 unit(transaction) the size of the list*/
        
        printf("[USER CHILD #%d] Trans con amount %d inviata al nodo %d",getpid(),tr.amount,pid);
        /*updateBudget(tr.amount, my_index);*/
        checkLedger(*trans_sent);
        fprintf(fd,"\n[USER #%d] Transazione %d°:\n\tSender:%d\n\tReceiver:%d\n\tTimestamp Sec:%ld  NSec:%ld\n\tReward:%d\n\tAmount:%d\n",getpid(),trans_sent_Index,tr.sender,tr.receiver,tr.timestamp.tv_sec,tr.timestamp.tv_nsec,tr.reward,tr.amount);

        printf("[USER CHILD #%d] Invio a USER #%d di %d\n",getpid(),tr.receiver,tr.amount);
        if(nanosleep(&time,NULL)== -1){
            TEST_ERROR
        }
        printf("[USER CHILD #%d] Current Budget:%d\n",getpid(),getBudget(my_index));
        }
    }

    shmdt(shm_buf);
    printf("[USER CHILD #%d] ABOUT TO ABORT\n",getpid());

    
    return 0;
}
/*
    Method that create a new transaction.
    If budget is less than 2, no transaction will be created. 
*/
 int creaTransazione(struct transaction* tr,int budget){
    if(budget <2){ 
        return -1;
    }else{
    struct timespec curr_time;
	int tmp_budget;
    int user;
    
    clock_gettime(CLOCK_REALTIME,&curr_time);
    tmp_budget=rand()%(budget+1-2) +2;/*rSelecting random budget in range [2,budget]. Cannot send more money than current budget*/
    do{
        user=(rand()%N_USERS);/*Selecting a random user in range [0,N_USERS-1]*/
    }while(pid_users[user].pid == getpid());/*This way I force sender not to send the transaction to itself*/
    

	tr->timestamp = curr_time;/*current clock_time*/
	tr->sender = getpid();/*PID of the process creating the transaction*/
	tr->receiver = pid_users[user].pid;/*choosing a random user in the range [0,N_USERS-1]. 12 is the offset(and number of macro) in the shm buffer*/
	
    /*commission for node process*/
    tr->reward=(int)((double)SO_REWARD/100*tmp_budget);/* percentage/100 * budget ie: 12% of 500 -> 12/100 * 500*/
    tr->reward= tr->reward == 0 ? 1:tr->reward;/*Reward has to be atleast 1*/

    tr->executed=1;

    tr->amount = tmp_budget - tr->reward;/*amount to be sent is equal to: tr.amount-(so_reward*amount)*/
    
    return 0;
    }
}
void printTransaction( transaction tr){
    printf("[USER CHILD #%d]Sender:%d\n",getpid(),tr.sender);
    printf("[USER CHILD #%d]Receiver:%d\n",getpid(),tr.receiver);
    printf("[USER CHILD #%d]Reward:%d\n",getpid(),tr.reward);
    printf("[USER CHILD #%d]TimestampSec:%ld  NSec:%ld\n",getpid(),tr.timestamp.tv_sec,tr.timestamp.tv_nsec);
    printf("[USER CHILD #%d]Amount:%d\n",getpid(),tr.amount);
}
int getRndNode(){
	return rand()%N_NODES;
}

 
/*TODO: il budget và calcolato controllando il libro mastro
  TODO: usare semafori per scrivere il budget  */
void updateBudget(int costoTransazione,  int my_index){
    int i=0,j=0;
    int budget=SO_BUDGET_INIT;
    int amount;
    transaction tmp;

    /*Subtracting the money of the transaction sent BUT NOT yet written in the ledger */
    for(i=0;i<trans_sent_Index;i++){
       tmp=trans_sent[i];
      /* if(checkLedger(trans_sent[i]) == 0){
           budget=budget - tmp.amount;
       }*/
       budget=budget-tmp.amount;
    }
    TEST_ERROR
    shm_info[my_index].budget = shm_info[my_index].budget - costoTransazione;
     
    
}

void updateInfos(int budget,int abort_trans,int my_index){
    shm_info[my_index].pid=getpid();
    shm_info[my_index].type=0;
    shm_info[my_index].budget=budget;
    shm_info[my_index].abort_trans=abort_trans;
}
/*FIXME: fixare handle_signal(errori sintattici etc)
  TODO: switch case con i vari segnali. se termina prematuramente(SO_RETRY ad esempio), impostare abort_trans a 1 */


int checkLedger(transaction tr){
    /*Method that checks the whole ledger and match if the transaction sent by the user is written in it
    Cosa deve fare esattamente:
        -Lock sul libro mastro per evitare inconsistenza dei dati
        
        -Controlla tutto il libro mastro, per ogni transazione dove il sender è lo user stesso
         decrementa il budget(inizializzato a SO_BUDGET_INIT) di amount+reward
        
        -Controlla tutto il libro mastro, per ogni transazione dove il receiver è lo user stesso
         aumenta il budget(inizializzato a SO_BUDGET_INIT) di amount

        -Controlla la lista trans_sent, per ogni transazione controlla che sia presente sul libro mastro
         se è così, questa transazione viene tolta dalla lista trans_sent
         se non è così, decrementa il budget di amount+reward

        -Rilascio del lock sul libro mastro

        FIXME: Budget negativo, calcolo sbagliato
*/
    int i=0,j=0,z=0;
    int found=0;/*0 : found   1: not found*/
    int budget=SO_BUDGET_INIT;
    transaction curr_tr;/*auxiliary variable so the code looks cleaner :)*/
    
    sops.sem_num=2;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);
    printf("#%d Parto con Budget:%d\n",getpid(),budget);
    for(i=0;i<SO_REGISTRY_SIZE;i++){
        if(mastro_area_memoria[i].executed == 1){
            for(j=0;j<SO_BLOCK_SIZE;j++){
                curr_tr = mastro_area_memoria[i].transactions[j];
                if(curr_tr.sender == getpid())
                    budget=budget- curr_tr.amount-curr_tr.reward;
                else if(curr_tr.receiver == getpid())
                    budget=budget+ curr_tr.amount;
                printf("#%d Budget aggiornato:%d\n",getpid(),budget);
        }
        }
    }
   
    for(z=0;z<trans_sent_Index;z++){
        printf("User #%d\n",getpid());
        printTransaction(trans_sent[z]);
         for(i=0;i<SO_REGISTRY_SIZE && found == 0;i++){
             if(mastro_area_memoria[i].executed == 1){
            for(j=0;j<SO_BLOCK_SIZE && found ==0;j++){
                curr_tr = mastro_area_memoria[i].transactions[j]; 
                if(curr_tr.sender == trans_sent[z].sender && curr_tr.timestamp.tv_nsec == trans_sent[z].timestamp.tv_nsec && curr_tr.timestamp.tv_sec == trans_sent[z].timestamp.tv_sec){
                    printf("devo rimuovere la transazione\n");
                    found=1;
                }
            }
            }
        }
        if(found == 0){
            budget=budget - trans_sent[z].amount - trans_sent[z].reward;
        }else{
            found=0;
        }
    }
     
    shm_info[my_index].budget=budget;
    printf("Sto per aggiornare il budget del mio pid:%d   budget:%d\n",shm_info[my_index].pid,budget);


    sops.sem_num=2;
    sops.sem_op=1;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);

    return 0;/*Transaction not found*/
}
/*Method copied from stack overflow xd*/
transaction * removeTrans(transaction  tr){
    int i=0,j=0;
    transaction* tmp;
    /*Finding the index of the transaction to remove and exiting the loop when found*/
    for(i=0;i<trans_sent_Index;i++){
        if(trans_sent[i].sender == tr.sender && trans_sent[i].timestamp.tv_nsec == tr.timestamp.tv_nsec && trans_sent[i].timestamp.tv_sec == tr.timestamp.tv_sec ){
            break;
        }
    }
    /*Storing all the transaction ,UP TO the transaction to remove, in temp*/
    tmp=malloc(sizeof(transaction)*(trans_sent_Index+1));/*Temp variable equal to trans_sent*/
    if(i!=0)
        memcpy(tmp,trans_sent,sizeof(transaction)*i);
    /*Copying the other transactions excluding the one to delete*/
    if(i!=(sizeof(transaction)*(i-1)))
        memcpy(tmp+(i*sizeof(transaction)),trans_sent+((i+1)*sizeof(transaction)),trans_sent_Index+1-i);
    free(trans_sent);

    trans_sent_Index=trans_sent_Index-1;
    return tmp;
}

void signalsHandler(int signal) {
    shmdt(shm_macro);
    shmdt(shm_info);
    
    printf("<<user>> %d ha pulito tutto :) \n", getpid());
    exit(EXIT_SUCCESS);
}