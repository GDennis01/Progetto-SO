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
msgqbuf msg_buf, master_buf;
int my_index=0;
FILE * debug_blocco_nodo;
int n_trans=0;
int blocks_written=0;

 void printblocco( transaction_block nuovoBlocco){
     int i=0;
     transaction tr;
     /*printf("Porcodio@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");*/
     fprintf(debug_blocco_nodo,"[NODE CHILD #%d] Il seguente blocco è stato scritto sul mastro:\n",getpid());
     for(i=0;i<SO_BLOCK_SIZE;i++){
         tr=nuovoBlocco.transactions[i];
        fprintf(debug_blocco_nodo,"\n[NODE CHILD #%d] Transazione %d°:\n\tSender:%d\n\tReceiver:%d\n\tTimestamp Sec:%ld  Nsec:%ld\n\tReward:%d\n\tAmount:%d\n",getpid(),i+1,tr.sender,tr.receiver,tr.timestamp.tv_sec,tr.timestamp.tv_nsec,tr.reward,tr.amount);

     }
 }
 void printTransaction( transaction tr){
    printf("[NODE CHILD #%d]Sender:%d\n",getpid(),tr.sender);
    printf("[NODE CHILD #%d]Receiver:%d\n",getpid(),tr.receiver);
    printf("[NODE CHILD #%d]Reward:%d\n",getpid(),tr.reward);
    printf("[NODE CHILD #%d]TimestampSec:%ld  NSec:%ld\n",getpid(),tr.timestamp.tv_sec,tr.timestamp.tv_nsec);
    printf("[NODE CHILD #%d]Amount:%d\n",getpid(),tr.amount);
    printf("[NODE CHILD #%d]Hops:%d\n",getpid(),tr.hops);
}
 int main(int argc, char const *argv[])
{

    int info_id,macro_id,sem_id,mastro_id,masterq_id,i,j;
    int budget;
    int sum_reward=0;
    int bytes_read;
    
    struct sigaction sa;
    transaction tr;
    transaction_block block;
    struct msqid_ds msg_ds;
    info_process infos;
    struct timespec time;
    int *my_friends;
    int pidfriend;
    int tempo;
    trans_pool pool;


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
    my_friends=malloc(sizeof(int)*SO_N_FRIENDS);
    
    /*Pool size is equal to the number of transaction maximum allowed(SO_TP_SIZE) times the size of a single transaction*/
    pool=malloc(SO_TP_SIZE*sizeof(*pool));
    
    masterq_id=msgget(getppid(), 0666);
    
    /*Fine ricezione amici*/
    /*printf("[NODE CHILD #%d] MY MSGQ_ID : %d\n",getpid(),msgq_id);*/
    /*The semaphore is used so that all nodes can create their queues without generating inconsistency*/
    sops.sem_num=1;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);

    for(i=0;i<SO_N_FRIENDS;i++){
        msgrcv(masterq_id,&msg_buf,sizeof(msg_buf.tr),getpid(),0);
        my_friends[i]=msg_buf.tr.receiver;
        /*printf("Son nodo %d e questo è il mio %d° amico:%d\n",getpid(),i,shm_info[N_USERS+my_friends[i]].pid);*/
        TEST_ERROR
    }

    
    sops.sem_num=3;
    sops.sem_op=0;
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

    while(1){
        while(n_trans < SO_TP_SIZE &&  ((bytes_read=msgrcv(masterq_id,&msg_buf,sizeof(msg_buf.tr),getpid(),IPC_NOWAIT)) > 0) /*&& errno != ENOMSG*/ ){
            /*If the trans pool is empty, im gonna insert the new trans in the head instead of the tail :D*/
            pool=(n_trans == 0)  ? insertHead(pool,msg_buf.tr) : insertTail(pool,msg_buf.tr);
            n_trans++;
        } 
        bytes_read=0;

        if(n_trans == SO_TP_SIZE){
            if((bytes_read=msgrcv(masterq_id,&master_buf,sizeof(master_buf.tr),getpid(),0)) >0){
                
                if(master_buf.tr.hops > 0){
                    pidfriend = shm_info[N_USERS+my_friends[rand()%SO_N_FRIENDS]].pid;
                    master_buf.mtype = pidfriend ; /*preso friend random nel modo più complesso possibile*/
                    master_buf.tr.hops = master_buf.tr.hops -1;
                    if(msgsnd(masterq_id,&master_buf,sizeof(master_buf.tr),IPC_NOWAIT) == -1)
                        fprintf(stderr,"HO FALLITO\n");
                    if(master_buf.tr.hops <3)
                        fprintf(stderr,"MINORE DI 3 SIUU\n");
                    printf("#####RIMBALZELLO: %d, pidfreidn: %d   Sender:%d  Receiver:%d  NSec:%ld\n", master_buf.tr.hops, pidfriend,master_buf.tr.sender,master_buf.tr.receiver,master_buf.tr.timestamp.tv_nsec);
                    printTransaction(master_buf.tr);    
                }else{
                /*mandala al father che ha hops 0 */

                printf("#####PADRE RIMBALALO TU\n");
                master_buf.mtype = getppid();
                if(msgsnd(masterq_id,&master_buf,sizeof(master_buf.tr),0) == -1){}
                }
            }
            
        }
        if(blocks_written>SO_REGISTRY_SIZE-1)
            {
                  kill( getppid() , SIGUSR1 );
                  raise(SIGTERM);
            }
        if(n_trans >= SO_BLOCK_SIZE -1){ 
            pool=createBlock(pool,&sum_reward,&block);
            /*If there's just one spot left on the block, I proceed to fill it with a reward transaction*/
            creaTransazione(&tr,sum_reward);/*Saving the reward transaction in the last spot of the block*/
            tr.executed=1;
            block.transactions[SO_BLOCK_SIZE-1]=tr;
            printblocco(block);
            scritturaMastro(sem_id,block);
        }
        nanosleep(&time,NULL);/*Simulating the elaboring process*/
        updateInfos(sum_reward,0,my_index,1);
        sum_reward=0;
        time.tv_sec=1;/*1 for debug mode*/
        time.tv_nsec=rand()%(MAX_TRANS_PROC_NSEC+1-MIN_TRANS_PROC_NSEC) +MIN_TRANS_PROC_NSEC;    

    /*Reading all transaction sent to me till noone are left*/
    



    /*bytes_read=msgrcv(msgq_id,&msg_buf,sizeof(msg_buf.tr),getpid(),0); */

    
    
    /*rimbalzo hops*/
   /* bytes_read=msgrcv(masterq_id,&master_buf,sizeof(master_buf.tr),getpid(),IPC_NOWAIT);*/
    if(/*bytes_read > 0*/ 0==1){
        tempo = master_buf.tr.hops;
        if(tempo > 0){
            pidfriend = shm_info[N_USERS+my_friends[rand()%SO_N_FRIENDS]].pid;
            master_buf.mtype = pidfriend ; /*preso friend random nel modo più complesso possibile*/
            tempo = tempo -1;
            if(msgsnd(msgget(pidfriend,0666),&master_buf,sizeof(master_buf.tr),IPC_NOWAIT) == -1){
                if(msgsnd(masterq_id,&master_buf,sizeof(master_buf.tr),IPC_NOWAIT) == -1){}
            }
            printf("#####RIMBALZELLO: %d, pidfreidn: %d   Sender:%d  Receiver:%d  NSec:%ld\n", tempo, pidfriend,master_buf.tr.sender,master_buf.tr.receiver,master_buf.tr.timestamp.tv_nsec);
        }else{
            /*mandala al father che ha hops 0 */

            /*printf("#####PADRE RIMBALALO TU\n");
            master_buf.mtype = getppid();
            if(msgsnd(masterq_id,&master_buf,sizeof(master_buf.tr),0) == -1){}*/
        }
    }

    }

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
    tr->hops=SO_HOPS;
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
        sops.sem_num=2;
        sops.sem_op=-1;
        sops.sem_flg=0;
        semop(semaforo_id,&sops,1); /*seize the resource, now it can write on the mastro*/

    /* l'indice "i" alla fine del ciclo avrà il valore dell'ultimo posto libero*/    
        while(mastro_area_memoria[i].executed == 1 && i<SO_REGISTRY_SIZE){  
            i++;
            
        }
        blocks_written=i;
        
       
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
        return 0;
    
}

void signalsHandler(int signal) {
    updateInfos(0,n_trans,my_index,0);
    shmdt(shm_info);
    shmdt(shm_macro);
    shmdt(mastro_area_memoria);
    printf("<<nodo>> %d ha pulito tutto :) \n", getpid());
    exit(EXIT_SUCCESS);
}



trans_pool createBlock(trans_pool p,int * sum_reward,transaction_block * block){
    int i=0;
    transaction* tmp;
    
    for(i=0;i<SO_BLOCK_SIZE-1;i++){
        *sum_reward= *sum_reward+p->reward;
        tmp=p->next;
        
        block->transactions[i]= (*p);
        
        transDeleteIf(p,*p);
        
        n_trans--;
        p=tmp;
    }
    return tmp;
}
