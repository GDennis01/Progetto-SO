#include "common.c"
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
/*-Creare una shared memory(vedi shmget) nel processo master con una chiave definita nel file macro.txt
-Fare execve dei processi user e nodo passando come argomento la chiave della shared memory
-Ogni processo user/nodo farà l'attach alla shmemory e leggerà il contenuto (vedere come leggere la roba dalla shared memory). Una volta finita la lettura si effettuerà il deattach
-Il processo master alla fine farà il deattach finale così da garantire la rimozione della shared memory
(L'ho scritto come promemoria per non dimenticarmelo)*/
#define SHM_KEY "12345"
#define SEM_KEY "11"

int main(int argc, char const *argv[])
{
    int  i,z,err,child_pid,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    int key,key2;/*key:key for shared memory key2:key for semaphores*/
    char *shm_key[]={SHM_KEY,NULL};
    struct shm_buf *buf;
    struct child * user_array=malloc(sizeof(struct child)*N_USERS);
    struct child * node_array=malloc(sizeof(struct child)*N_NODES);
    struct sembuf sops;



    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    key=shmget(atoi(SHM_KEY),sizeof(buf->mtext)+sizeof(struct child)*(N_USERS+N_NODES),IPC_CREAT| 0660);
    printf("ID della SHM:%d\n",key);
    buf=shmat(key,NULL,0);
    buf->children[0]=user_array;
    buf->children[1]=node_array;

    /*Writing Macros to shared memory*/
    for(z=0;z<N_MACRO;z++){
        buf->mtext[z]=macros[z];
    }
   
    key2=semget(atoi(SEM_KEY),N_USERS,IPC_CREAT |0600);
    
    semctl(key2,0,SETVAL,N_USERS);
    TEST_ERROR
    
    printf("Dimensione testo struttura:%ld\n",sizeof(buf->mtext));
    TEST_ERROR
/*
    for(z=0;z<N_MACRO;z++){
        printf("Macro %d°:%d\n",z+1,buf->mtext[z]);
    }*/
    for(i=0;i<N_USERS;i++){
        switch(child_pid=fork()){
            /*To avoid inconsistency reading from user processes, I stop said processes at a semaphore till the father is done writing PIDs in the shm array*/
            case 0:
                sops.sem_num=0;
                sops.sem_op=0;
                sops.sem_flg=0;

                semop(key2,&sops,1);/*wait for 0 operation*/

                execve("User",shm_key,NULL);
                TEST_ERROR
            break;

            /*Parent code*/
            default:
            sops.sem_num=0;
            sops.sem_op=-1;
            sops.sem_flg=0;

            buf->children[0][i].pid=child_pid;
            buf->children[0][i].status=1;

            printf("PID FATHER:%d     PID CHILD:%d\n",getpid(),buf->children[0][i].pid);
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
            /*execveing  node processes*/
            break;

            /*Parent code*/
            default:
            /*handling stuff*/
            break;
        }
    }
    /*shmctl(key,IPC_RMID,NULL)*/
    return 0;
}
/*TODO: FIXARE LA ROBA DEGLI SPAZI IN MACROS.TXT*/


