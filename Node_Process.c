/*
   
	Ultime modifiche:
    -10/01/2022
        -Ora updateInfos aggiorna correttamente lo stato del processo(vivo o morto)
    -08/01/2022
        -Indice al blocco
    -05/01/2022
        -Ora esegue correttamente il detach dalle shared memories
        -Cambiato il modo in cui veniva generato il tempo randomico. Prima poteva dare overflow
        -Ora il nodo salva correttamente nella shared memory il proprio bilancio.
         Il problema era dovuto al fatto che il nodo non si salvava correttamente il proprio indice
        -Aggiornato il metodo "creaTransazione()", ora restituisce un intero
        -Se la nuova dimensione della coda risulta esser maggiore della massima consentita, allora 
         non la aggiorno.

    -03/01/2022
        -Ora ci sono solo tre key: info,macro e sem
        -Le macro ora vengono salvate in *shm_macro, le info dei processi in *info
        -Impostata la dimensione massima della coda di messaggi(transazione*TP_SIZE)
        -Implementato il ciclo(while(1))
        -Implementata la scrittura sul libro mastro
        
	-30/12/2021
		-Aggiunta del metodo "creaTransazione"
        -Aggiunta del metodo "scritturaMastro"
*/
#include "Node_Process.h"
#define IS_SENDER -1
int macros[N_MACRO];
info_process *pid_users;
info_process *pid_nodes;
 struct sembuf sops;
 int msgq_id;
 msgqbuf msg_buf;
 int my_index=0;
 FILE * debug_blocco_nodo;

 void printblocco(struct transaction_block nuovoBlocco){
     int i=0;
     transaction tr;
     fprintf(debug_blocco_nodo,"[NODE CHILD #%d] Il seguente blocco è stato scritto sul mastro:\n",getpid());
     for(i=0;i<SO_BLOCK_SIZE;i++){
         tr=nuovoBlocco.transactions[i];
        fprintf(debug_blocco_nodo,"\n[NODE CHILD #%d] Transazione %d°:\n\tSender:%d\n\tReceiver:%d\n\tTimestamp Sec:%ld  Nsec:%ld\n\tReward:%d\n\tAmount:%d\n",getpid(),i+1,tr.sender,tr.receiver,tr.timestamp.tv_sec,tr.timestamp.tv_nsec,tr.reward,tr.amount);

     }
 }
 int main(int argc, char const *argv[])
{

    int info_id,macro_id,sem_id,mastro_id,i,j;
    int budget;
    
    int sum_reward=0;
    int bytes_read;
    struct sigaction sa;
    transaction tr;
    transaction_block block;
    struct msqid_ds msg_ds;
    
    info_process infos;
    struct timespec time;

    /* signal handler */
    bzero(&sa,sizeof(sa));

    sa.sa_handler=signalsHandler;
    sigaction(SIGUSR1,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);
    
    debug_blocco_nodo=fopen("blocchi.txt","a+");
    
    
    info_id=atoi(argv[1]);
    shm_info=shmat(info_id,NULL,0666);/*Attaching to shm with info related to processes*/
    
    macro_id=atoi(argv[2]);
    shm_macro=shmat(macro_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    sem_id=atoi(argv[3]);

    mastro_id=atoi(argv[4]);
    mastro_area_memoria=shmat(mastro_id,NULL,0666);

  
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_macro[i];
    }

    
    msgq_id=msgget(getpid(),IPC_CREAT | 0666);
    /*16384 is the max default size of message queue*/
    if(sizeof(transaction)*SO_TP_SIZE < 16384){ 
        msg_ds.msg_qbytes=sizeof(transaction)*SO_TP_SIZE;
        msg_ds.msg_perm.mode=438;
        msgctl(msgq_id,IPC_SET,&msg_ds);/*Setting the size of the message queue equal to SO_TP_SIZE*/
        TEST_ERROR
    }

  

    /*printf("[NODE CHILD #%d] MY MSGQ_ID : %d\n",getpid(),msgq_id);*/
    /*The semaphore is used so that all nodes can create their queues without generating inconsistency*/
    sops.sem_num=1;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);

    /*
    printf("[NODE CHILD #%d] SEMVAL:%d\n",getpid(),semctl(sem_id,1,GETVAL));
    printf("[NODE CHILD #%d] ID della SHM:%d\n",getpid(),macro_id);
    */
    

    /*Populating the pid user array*/
    pid_users=malloc(sizeof(info_process)*N_USERS);
    for(i=0;i<N_USERS;i++){
        pid_users[i].pid=shm_info[i].pid;  
        pid_users[i].type=0;
        /*printf("[USER CHILD #%d] Leggo Users #%d \n",getpid(),pid_users[i].pid);    */

    }

    /*Populating the pid node array*/
    pid_nodes=malloc(sizeof(info_process)*N_NODES);
    for(i=0;i<N_NODES;i++){
        pid_nodes[i].pid=shm_info[i+N_USERS].pid;  
        pid_nodes[i].type=1;
        if(shm_info[i+N_USERS].pid==getpid()){
            my_index=i+N_USERS;
        }
        /*printf("[USER CHILD #%d] Leggo Nodo #%d \n",getpid(),pid_nodes[i].pid); */   
    }
    i=0;
    time.tv_sec=1;/*1 for debug mode */
    time.tv_nsec=rand()%(MAX_TRANS_PROC_NSEC+1-MIN_TRANS_PROC_NSEC) +MIN_TRANS_PROC_NSEC;/*[MIN_TRANS_PROC,MAX_TRANS_PROC]*/
    TEST_ERROR
    while(1){
    TEST_ERROR 
    if(i==SO_BLOCK_SIZE-1){/*If there's just one spot left on the block, I proceed to fill it with a reward transaction*/
       creaTransazione(&tr,sum_reward);/*Saving the reward transaction in the last spot of the block*/
        tr.executed=1;
        block.transactions[i]=tr;

        scritturaMastro(sem_id,block);
        nanosleep(&time,NULL);/*Simulating the elaboring process*/
        updateInfos(sum_reward,0,my_index,1);
        /*Resetting the variables for the next cycles*/
        i=0;
        sum_reward=0;
        time.tv_sec=1;/*1 for debug mode*/
        time.tv_nsec=rand()%(MAX_TRANS_PROC_NSEC+1-MIN_TRANS_PROC_NSEC) +MIN_TRANS_PROC_NSEC;    
    }

    bytes_read=msgrcv(msgq_id,&msg_buf,sizeof(msg_buf.tr),getpid(),0);

    if(bytes_read >= 32){/*It reads only 32 bytes/transaction aka if the msgrcv went well*/
        sum_reward=sum_reward+msg_buf.tr.reward;
        block.transactions[i]=msg_buf.tr;/*Saving the transaction just read in the block to-be-executed*/
        i++;
    }
    /*printf("[NODE CHILD #%d] LETTI %d BYTES\n",getpid(),bytes_read);*/
    }

    /*
    msgctl(msgq_id,IPC_RMID,NULL);*/
    printf("[NODE CHILD] ABOUT TO ABORT\n");

    return 0;
}

/*
    Method that updates the info of the current node
*/
void updateInfos(int budget,int abort_trans,int index,int isAlive){
    shm_info[index].budget+=budget;
    shm_info[index].abort_trans=abort_trans;
    shm_info[index].alive=isAlive;
}
/*
    Creation of the reward transaction
*/
int creaTransazione(struct transaction* tr,int budget){
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME,&curr_time);
    tr->timestamp = curr_time;/*current clock_time*/
	tr->sender = IS_SENDER;/*MACRO DA DEFINIRE*/
	tr->receiver = getpid();
	tr->amount = budget;
    /*commission for node process*/
    tr->reward=0;
    tr->executed = 1 ;
    return 0;
}

/*
    Controlla se il libro mastro è pieno
    Se è pieno allora manda un segnale al padre dicendo di terminare tutto
    (una delle condizione di terminazione è appunto il libro mastro pieno)
    se non è pieno, scrive il blocco nel libro mastro.
*/
int scritturaMastro(int semaforo_id, struct transaction_block nuovoBlocco){
    int i=0;
 /* l'indice "i" alla fine del ciclo avrà il valore dell'ultimo posto libero*/    
    while(mastro_area_memoria[i].executed == 1 && i<SO_REGISTRY_SIZE){  
        i++;
    }
    if(i>SO_REGISTRY_SIZE-1){ /*se il posto libero è oltre i confini del mastro, vuol dire che quest'ultimo è pieno */
        kill( getppid() , SIGUSR1 );
        raise(SIGTERM);/*Mi ammazzo prima che mi ammazzi il padre*/
        return 0;
    }else{
       
        sops.sem_num=2;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(semaforo_id,&sops,1); /*seize the resource, now it can write on the mastro*/
        mastro_area_memoria[i] = nuovoBlocco;
        mastro_area_memoria[i].id=i;
        mastro_area_memoria[i].executed=1;
        mastro_area_memoria[i+1].executed=0; 

        /*printf("Io, Nodo #%d, ho scritto nella %d° posizione del libro mastro:\n",getpid(),i+1);*/
       /* printblocco(nuovoBlocco);*/
        /* restituzione del semaforo*/
        sops.sem_num=2;
        sops.sem_op=1;
        sops.sem_flg=0;
        semop(semaforo_id,&sops,1); /*gives back the resource, now other nodes can write on the mastro*/
        return 1;
    }
}

void signalsHandler(int signal) {
    int trans_left=0;
     struct msqid_ds buf;
    /*Counting how many transactions are left in the transaction pool(aka message queue)*/
    msgctl(msgq_id,IPC_STAT,&buf);
    trans_left=buf.msg_qnum;
    TEST_ERROR
    printf("Il valore dei trans è:%ld\n",buf.msg_qnum);

    updateInfos(0,trans_left,my_index,0);
    shmdt(shm_info);
    shmdt(shm_macro);
    shmdt(mastro_area_memoria);
    msgctl(msgq_id,IPC_RMID,NULL);
    printf("<<nodo>> %d ha pulito tutto :) \n", getpid());
    exit(EXIT_SUCCESS);
}



