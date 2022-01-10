#include "common.c"
 /* Per poter compilare con -std=c89 -pedantic */
/*
GESTIONE NODI:
Problema: Il master può creare N_NODES + X dove X varia con il crescere del tempo.
Dove salvo questi X nodi? Dovrei avere una shared memory con una dimensione che varia nel tempo -> Impossibile

Soluzione pensata: allocare uno spazio extra nella shared memory. Questa servirà per salvare temporaneamente
il nuovo nodo creato. Il master, gli user e i nodi se lo salveranno localmente ogni volta che questo viene generato

Problema 2: I nuovi nodi creati come fanno a tener traccia degli X-1 nodi creati prima di lui? 
Solo i nodi "originali" riescono ad aver traccia di TUTTI i nodi creati

Soluzione pensata: L'X-esimo nodo creato, per salvarsi gli X-1 nodi creati prima di lui, leggerà da
una coda di messaggi, il messaggio con tipo=getpid() contenente tutti i pid degli X-1 nodi creati prima di lui.
Se ne occuperà il master di inviare un messaggio, di tipo getpid()(dove il pid è quello del nuovo nodo creato)
contenente tutti i nuovi nodi creati prima di lui.

P.S. : Per nuovi nodi creati, si intendono quelli generati quando la transaction pool di ogni nodo "originale" è piena
*/
/*
    Ultime modifiche
        -03/01/2022
            -Modifica al metodo initIPCs()
            -Ora ci sono solo tre key: info,macro e sem
            -Le macro vengono salvate in *shm_macro, le info dei processi in *info
            -alarmHandler -> signalsHandler
            -Aggiunto mastro_key e la sua relativa shared
*/

int dims=0;
int info_key,macro_key,sem_key,mastro_key;/*key:key for shared memory key2:key for semaphores  key3:key for message queue*/

int main(int argc, char const *argv[])
{
    struct sigaction sa;
    int  i,status,err,child_pid,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    char str[15],str2[15],str3[15],str4[15];/*Stringified key for shm*/
    char *arguments[]={NULL,NULL,NULL,NULL,NULL,NULL};/*First arg of argv should be the filename by default*/
    struct sembuf sops;

    bzero(&sa,sizeof(sa));

    sa.sa_handler=signalsHandler;
    sigaction(SIGALRM,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);

    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    dims=sizeof(info_process)*(N_USERS+N_NODES);

    initIPCS(&info_key,&macro_key,&sem_key,&mastro_key, dims);
    shm_info=shmat(info_key,NULL,0660);
    shm_macro=shmat(macro_key,NULL,0660);
    mastro_area_memoria = (transaction_block*)shmat(mastro_key, NULL, 0);  /*così leggi a blocchi di transazione*/


    if(info_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(macro_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(sem_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(mastro_key==-1){
        TEST_ERROR
        exit(1);
    }


    TEST_ERROR
    printf("[PARENT #%d] ID della SHM_INFO:%d\n",getpid(),info_key);
    printf("[PARENT #%d] ID del SEM:%d\n",getpid(),sem_key);
    
    sprintf(str,"%d",info_key);/*I convert the key from int to string*/
    arguments[1]=str;
    
    sprintf(str2,"%d",macro_key);/*I convert the key from int to string*/
    arguments[2]=str2;/*Had to create another temporary string(str2) due to strange behaviour with sprintf*/
    
    sprintf(str3,"%d",sem_key);/*I convert the key from int to string*/
    arguments[3]=str3;

    sprintf(str4,"%d",mastro_key);/*I convert the key from int to string*/
    arguments[4]=str4;

  
    
    /*Writing Macros to shared memory.
    The first 12 locations of the arrays are reserved for the macros,others will be used
    to store pids and statuses of user/node processes*/
    
    for(i=0;i<N_MACRO;i++){
        shm_macro[i]=macros[i];
    }

    semctl(sem_key,1,SETVAL,N_NODES);
    /*Generating user children*/
    semctl(sem_key,0,SETVAL,N_USERS);/*Semaphore used to synchronize writer(master) and readers(nodes/users)*/
    semctl(sem_key,2,SETVAL,1);/*Semaphore used to synchronize nodes writing on mastro*/
   
    alarm(SO_SIM_SEC);

    for(i=0;i<N_USERS;i++){
        switch(child_pid=fork()){
            /*To avoid inconsistency reading from user processes, I stop said processes at a semaphore till the father is done writing PIDs in the shm array*/
            case 0:
                sops.sem_num=0;
                sops.sem_op=0;
                sops.sem_flg=0;

                semop(sem_key,&sops,1);/*wait for 0 operation*/
                arguments[0]="User";
                execve("User",arguments,NULL);
                exit(1);
            break;

            /*Parent code*/
            default:

                shm_info[i].pid=child_pid;
                shm_info[i].type=0;
                shm_info[i].budget=SO_BUDGET_INIT;
                
                /*printf("[PARENT #%d] USER CHILD appena creato ha pid %d\n",getpid(),shm_buf[N_MACRO+i].pid);*/
                sops.sem_num=0;
                sops.sem_op=-1;
                sops.sem_flg=0;
                semop(sem_key,&sops,1);/*decreasing the value by 1*/
            break;
        }
    }

    /*Generating children node processes*/
    semctl(sem_key,0,SETVAL,N_NODES);
    for(i=0;i<N_NODES;i++){
        switch(child_pid=fork()){
            /*Child code*/
            case 0:
            sops.sem_num=0;
            sops.sem_op=0;
            sops.sem_flg=0;
            semop(sem_key,&sops,1);/*wait for 0 operation*/
            
            arguments[0]="Node";
            execve("Node",arguments,NULL);
            exit(1);
            break;

            /*Parent code*/
            default:
  
            shm_info[N_USERS+i].pid=child_pid;
            shm_info[N_USERS+i].type=1;

            /*printf("[PARENT #%d] NODE CHILD appena creato ha pid %d\n",getpid(),shm_buf[N_MACRO+N_USERS+i].pid);*/
            
            sops.sem_num=0;
            sops.sem_op=-1;
            sops.sem_flg=0;
            semop(sem_key,&sops,1);/*decreasing the value by 1*/
            break;
        }
    }
    while(wait(&status) != -1);/*Waiting for all children to DIE*/
        
        
    
    printf("[PARENT] AFTER WAIT\n");
    deleteIPCs(info_key,macro_key,sem_key,mastro_key);
    printf("[PARENT] ABOUT TO ABORT\n");
    
    return 0;
}

void terminazione(info_process * shm_info,int reason,int dim){
    int i=0,cnt=0;
    printf("Il motivo della terminazione è %s \n",reason==0?"E' scaduto il tempo della simulazione":reason ==1?"La capacità del libro mastro si è esaurita":reason ==2?"Tutti i processi utenti sono terminati":"Motivo della terminazione ignoto. Errore");
    for(i=0;i<dim;i++){
        if(shm_info[i].type==0){
            printf("[User Process #%d]\nBilancio:%d\nTerminato prematuramente:%s\n",shm_info[i].pid,shm_info[i].budget,shm_info[i].abort_trans==1?"Sì":"No");
            if(shm_info[i].abort_trans==1)
                cnt++;
            kill( shm_info[i].pid , SIGTERM ); /*Killing every children*/
        }else if(shm_info[i].type==1){
            printf("[Node Process #%d]\nBilancio:%d\nTransazioni rimanenti nella transaction pool:%d\n",shm_info[i].pid,shm_info[i].budget,shm_info[i].abort_trans);
            kill( shm_info[i].pid , SIGTERM ); /*Killing every children*/
        }else printf("Errore, processo sconociuto\n");
         
    }
    deleteIPCs(info_key,macro_key,sem_key,mastro_key);
    printf("Numero di Processi Utenti terminati prematuramente:%d\n",cnt);
}


void signalsHandler(int signal) {
    switch(signal){
    case SIGALRM:
        terminazione(shm_info,0,dims);
        break;
    case SIGUSR1: /*Libro mastro is full*/
        terminazione(shm_info,1,dims);
        break;
    default:
        printf("Bruh\n");
    }
}
