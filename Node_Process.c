/*
    TODO: Mettere una dimensione subito dopo aver fatto la msgget

	Ultime modifiche:
    -03/01/2021
        -Ora ci sono solo tre key: info,macro e sem
        -Le macro ora vengono salvate in *shm_macro, le info dei processi in *info
        -Impostata la dimensione massima della coda di messaggi(transazione*TP_SIZE)
        
	-30/12/2021
		-Aggiunta del metodo "creaTransazione"
        -Aggiunta del metodo "scritturaMastro"
*/
#include "common.c"

int macros[N_MACRO];
info_process *pid_users;
info_process *pid_nodes;
 int main(int argc, char const *argv[])
{
    int info_id,macro_id,msgq_id,sem_id,mastro_id,i,j;
    int budget;
    int bytes_read;
    transaction tr;
    struct msqid_ds msg_ds;
    msgqbuf msg_buf;
    info_process infos;
    struct sembuf sops;

    /*Initializing infos that will be sent every second to the master process*/
    infos.pid=getpid();
    infos.type=1;
    infos.budget=0;
    infos.abort_trans=0;
    
    info_id=atoi(argv[1]);
    shm_info=shmat(info_id,NULL,SHM_RDONLY);/*Attaching to shm with info related to processes*/
    
    macro_id=atoi(argv[2]);
    shm_macro=shmat(macro_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    sem_id=atoi(argv[3]);

    mastro_id=atoi(argv[4]);
  
    msgq_id=msgget(getpid(),IPC_CREAT | 0660);
    msg_ds.msg_qbytes=sizeof(transaction)*SO_TP_SIZE;
    msgctl(msgq_id,IPC_SET,&msg_ds);
    TEST_ERROR
    
/*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_macro[i];

    }

    printf("[NODE CHILD #%d] MY MSGQ_ID : %d\n",getpid(),msgq_id);
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
        pid_users[i].type=1;
        /*printf("[USER CHILD #%d] Leggo Nodo #%d \n",getpid(),pid_nodes[i].pid); */   
    }

    bytes_read=msgrcv(msgq_id,&msg_buf,sizeof(msg_buf.tr),getpid(),0);
    printf("[NODE CHILD #%d] LETTI %d BYTES\n",getpid(),bytes_read);

    shmdt(&shm_buf);
    msgctl(msgq_id,IPC_RMID,NULL);
    printf("[NODE CHILD] ABOUT TO ABORT\n");

    return 0;
}

/*
    Method that updates the info of the current node
*/
void updateInfos(int budget,int abort_trans,info_process* infos){
    infos->budget=budget;
    infos->abort_trans=abort_trans;
}
/*
    Creation of the reward transaction
*/
struct transaction creaTransazione(unsigned int budget){
    transaction tr;
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME,&curr_time);
    srand(curr_time.tv_nsec);
    tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.sender = -1;/*MACRO DA DEFINIRE*/
	tr.receiver = getpid();
	tr.amount = budget;
    /*commission for node process*/
    tr.reward=0;
    tr.executed = 1 ;
    return tr;
}

/*
    Controlla se il libro mastro è pieno
    Se è pieno allora manda un segnale al padre dicendo di terminare tutto
    (una delle condizione di terminazione è appunto il libro mastro pieno)
    se non è pieno, scrive il blocco nel libro mastro.
*/
int scritturaMastro(){
    int i=0;
    while(mastro_area_memoria[i].executed == 1){
        if(i>SO_REGISTRY_SIZE-1)
            kill( getppid() , SIGUSR1 );
        i++;
    }
    /*
        Scrittura del blocco nel libro_mastro[i] (l'indici "i" alla fine del ciclo avrà il valore dell'ultimo posto libero)
    */
}




