#include "common.c"
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
/*-Creare una shared memory(vedi shmget) nel processo master con una chiave definita nel file macro.txt
-Fare execve dei processi user e nodo passando come argomento la chiave della shared memory
-Ogni processo user/nodo farà l'attach alla shmemory e leggerà il contenuto (vedere come leggere la roba dalla shared memory). Una volta finita la lettura si effettuerà il deattach
-Il processo master alla fine farà il deattach finale così da garantire la rimozione della shared memory
(L'ho scritto come promemoria per non dimenticarmelo)*/
#define SHM_KEY "123456"
#define SEM_KEY "11"
#define MASTRO_KEY "777"
#define MSGQUEUE_KEY "888"

int main(int argc, char const *argv[])
{
    int  i,z,err,child_pid,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    char str[15];/*Stringified key for shm*/
    int key,key2,keyLibroMastro,keyMsg;/*key:key for shared memory key2:key for semaphores*/
    char *userArgs[]={"User",NULL,NULL,NULL};/*First arg of argv should be the filename by default*/
    char *nodeArgs[]={"Node",NULL,NULL,NULL};
    struct sembuf sops;

    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    key = shmget(atoi(SHM_KEY),sizeof(macros)+sizeof(child)*(N_USERS+N_NODES),IPC_CREAT| 0660);
    TEST_ERROR
    printf("PADRE -> ID della SHM:%d\n",key);
    keyLibroMastro = shmget(atoi(MASTRO_KEY),SO_REGISTRY_SIZE*SO_BLOCK_SIZE*sizeof(transaction),IPC_CREAT| 0660); /*SO_REGISTRY E SO_BLOCK SONO LETTE A TEMPO DI COMPILAZIONE, SIZE DI TRANSACTION? dovrebbe fuznionare essendo definita in common.h*/
    TEST_ERROR
    printf("PADRE -> ID del libro mastro:%d\n",keyLibroMastro);
    sprintf(str,"%d",key);/*I convert the key from int to string*/
    userArgs[1]=str;
    nodeArgs[1]=str;
    sprintf(str,"%d",keyLibroMastro);/*I convert the key from int to string*/
    userArgs[2]=str;	/* ? */
    nodeArgs[2]=str;	/* ? */

    shm_buf=(child*)shmat(key,NULL,0);
	mastro_buf = (transaction*)shmat(keyLibroMastro, NULL, 0);
	
	/* apertura msgqueues dove users ficcano transazioni e i nodes le raccolgono */
	keyMsg = msgget( MSGQUEUE_KEY, IPC_CREAT|0660 );
	sprintf(str,"%d",keyMsg);/*I convert the key from int to string*/
    	userArgs[3]=str;	/* ? */
    	nodeArgs[3]=str;	/* ? */


    /*Writing Macros to shared memory*/
    for(z=0;z<N_MACRO;z++){
        shm_buf[z].pid=macros[z];
    }
    
    key2=semget(atoi(SEM_KEY),1,IPC_CREAT |0600);
    
    semctl(key2,0,SETVAL,N_USERS + N_NODES);
    TEST_ERROR

    /*for(z=0;z<N_MACRO;z++){
        printf("Macro %d°:%d\n",z+1,shm_buf[z].pid);
    }*/
    printf("FINE STAMPA PADRE\n");
    for(i=0;i<N_USERS;i++){
        switch(child_pid=fork()){
            /*To avoid inconsistency reading from user processes, I stop said processes at a semaphore till the father is done writing PIDs in the shm array*/
            case 0:
                sops.sem_num=0;
                sops.sem_op=0;
                sops.sem_flg=0;

                semop(key2,&sops,1);/*wait for 0 operation*/
                execve("User",userArgs,NULL);
                TEST_ERROR
            break;

            /*Parent code*/
            default:
                /*N_MACRO is the offset*/
                shm_buf[N_MACRO+i].pid=child_pid;
                shm_buf[N_MACRO+i].status=1;

                printf("Figlio appena creato ha pid %d\n",shm_buf[N_USERS+i].pid);
                sops.sem_num=0;
                sops.sem_op=-1;
                sops.sem_flg=0;
                TEST_ERROR
                semop(key2,&sops,1);/*decreasing the value by 1*/

                
                TEST_ERROR
            /*handling stuff*/
            break;
        }
    }

    for(i=0;i<N_NODES;i++){
        switch(fork()){
            /*Child code*/
            case 0:
            	sops.sem_num=0;
            	sops.sem_op=0;
           	sops.sem_flg=0;

            	semop(key2,&sops,1);/*wait for 0 operation*/
            	execve("Node",nodeArgs,NULL);
            	TEST_ERROR
            break;

            /*Parent code*/
            default:
            	shm_buf[N_MACRO+N_USERS+i].pid=child_pid;
                shm_buf[N_MACRO+N_USERS+i].status=1;

                printf("Figlio appena creato ha pid %d\n",shm_buf[N_USERS+N_USERS+i].pid);
                sops.sem_num=0;
                sops.sem_op=-1;
                sops.sem_flg=0;
                TEST_ERROR
                semop(key2,&sops,1);/*decreasing the value by 1*/

                
                TEST_ERROR
            /*handling stuff*/
            break;
        }
    }
    /*finita la creazione dei nodi sia users che nodes sono liberi di iniziare ad agire secondo il loro codice
    /*shmctl(key,IPC_RMID,NULL)*/
    while(wait(NULL) != -1);/*Waiting for all children to DIE*/
    msgctl(keyMsg, IPC_RMID , NULL ) ; /*RIMOZIONE MESSAGE QUEUE */
    return 0;
}
/*TODO: FIXARE LA ROBA DEGLI SPAZI IN MACROS.TXT*/


