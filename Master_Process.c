#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "common.c"
/*-Creare una shared memory(vedi shmget) nel processo master con una chiave definita nel file macro.txt
-Fare execve dei processi user e nodo passando come argomento la chiave della shared memory
-Ogni processo user/nodo farà l'attach alla shmemory e leggerà il contenuto (vedere come leggere la roba dalla shared memory). Una volta finita la lettura si effettuerà il deattach
-Il processo master alla fine farà il deattach finale così da garantire la rimozione della shared memory
(L'ho scritto come promemoria per non dimenticarmelo)*/
#define SHM_KEY "1234"
#define N_MACRO 12 /*Numbers of macros to read from "macros.txt"*/
/*Macros*/
#define N_USERS macros[0]
#define N_NODES macros[1]
#define MIN_TRANS_GEN_NSEC macros[2]
#define MAX_TRANS_GEN_NSEC macros[3]
#define MIN_TRANS_PROC_NSEC macros[4]
#define MAX_TRANS_PROC_NSEC macros[5]
#define SO_BUDGET_INIT macros[6]
#define SO_REWARD macros[7]
#define SO_RETRY macros[8]
#define SO_TP_SIZE macros[9]
#define SO_N_FRIENDS macros[10]
#define SO_SIM_SEC macros[11]



int main(int argc, char const *argv[])
{
    int  i,z,err,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    int key;
    char *shm_key[]={SHM_KEY,NULL};
    struct shm_buf *buf;

    
    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    key=shmget(atoi(SHM_KEY),sizeof(buf->mtext),IPC_CREAT| 0660);
    printf("ID della SHM:%d\n",key);
    buf=shmat(key,NULL,0);

    /*Writing Macros to shared memory*/
    for(z=0;z<N_MACRO;z++){
        buf->mtext[z]=macros[z];
    }
    
    printf("Dimensione testo struttura:%ld\n",sizeof(buf->mtext));
    TEST_ERROR
/*
    for(z=0;z<N_MACRO;z++){
        printf("Macro %d°:%d\n",z+1,buf->mtext[z]);
    }*/
    for(i=0;i<N_USERS;i++){
        switch(fork()){
            /*Child code*/
            case 0:
            /*execveing  user processes*/
            /*execve("User",shm_key,NULL);*/
            err=execve("User",shm_key,NULL);
            if(err==-1)
                TEST_ERROR
            break;

            /*Parent code*/
            default:
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


