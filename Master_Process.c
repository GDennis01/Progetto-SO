#include "common.c"
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
/*-Creare una shared memory(vedi shmget) nel processo master con una chiave definita nel file macro.txt
-Fare execve dei processi user e nodo passando come argomento la chiave della shared memory
-Ogni processo user/nodo farà l'attach alla shmemory e leggerà il contenuto (vedere come leggere la roba dalla shared memory). Una volta finita la lettura si effettuerà il deattach
-Il processo master alla fine farà il deattach finale così da garantire la rimozione della shared memory
(L'ho scritto come promemoria per non dimenticarmelo)

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
#define SHM_KEY "123456"
#define SEM_KEY "11"

int main(int argc, char const *argv[])
{
    int  i,z,err,child_pid,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO],dim;
    char str[15],str2[15];/*Stringified key for shm*/
    int shm_key,sem_key,msgq_key;/*key:key for shared memory key2:key for semaphores  key3:key for message queue*/
    char *arguments[]={NULL,NULL,NULL,NULL};/*First arg of argv should be the filename by default*/
    struct sembuf sops;

    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    dim=sizeof(macros)+sizeof(child)*(N_USERS+N_NODES)+1;

    initIPCS(&shm_key,&sem_key,&msgq_key,dim);
    if(shm_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(sem_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(msgq_key==-1){
        TEST_ERROR
        exit(1);
    }


    TEST_ERROR
    printf("[PARENT #%d] ID della SHM:%d\n",getpid(),shm_key);
    printf("[PARENT #%d] ID del SEM:%d\n",getpid(),sem_key);
    sprintf(str,"%d",shm_key);/*I convert the key from int to string*/
    arguments[1]=str;
    
    sprintf(str2,"%d",msgq_key);/*I convert the key from int to string*/
    arguments[2]=str2;/*Had to create another temporary string(str2) due to strange behaviour with sprintf*/

    shm_buf=(child*)shmat(shm_key,NULL,0);/*Attaching the shm_buf variable to shared memory*/
    
    /*Writing Macros to shared memory.
    The first 12 locations of the arrays are reserved for the macros,others will be used
    to store pids and statuses of user/node processes*/
    
    for(z=0;z<N_MACRO;z++){
        shm_buf[z].pid=macros[z];
    }
    /*Generating user children*/
    semctl(sem_key,0,SETVAL,N_USERS);/*Semaphore used to synchronize writer(master) and readers(nodes/users)*/
   
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
                /*N_MACRO is the offset used to calculate the first pid in shm struct*/
                shm_buf[N_MACRO+i].pid=child_pid;
                shm_buf[N_MACRO+i].status=1;
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
            
            shm_buf[N_MACRO+N_USERS+i].pid=child_pid;
            shm_buf[N_MACRO+N_USERS+i].status=1;

            /*printf("[PARENT #%d] NODE CHILD appena creato ha pid %d\n",getpid(),shm_buf[N_MACRO+N_USERS+i].pid);*/
            
            sops.sem_num=0;
            sops.sem_op=-1;
            sops.sem_flg=0;
            semop(sem_key,&sops,1);/*decreasing the value by 1*/
            break;
        }
    }
    
    while(wait(NULL) != -1);/*Waiting for all children to DIE*/
    printf("[PARENT] AFTER WAIT\n");
   /* shmctl(shm_key,IPC_RMID,NULL);/*Detachment of shared memory
    semctl(sem_key,0,IPC_RMID,NULL);/*Deleting semaphore
    msgctl(msgq_key,IPC_RMID,NULL);*/
    deleteIPCs(shm_key,sem_key,msgq_key);
    printf("[PARENT] ABOUT TO ABORT\n");
    
    return 0;
}


