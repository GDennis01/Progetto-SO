#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "Master_Process.h"

/*-Creare una shared memory(vedi shmget) nel processo master con una chiave definita nel file macro.txt
-Fare execve dei processi user e nodo passando come argomento la chiave della shared memory
-Ogni processo user/nodo farà l'attach alla shmemory e leggerà il contenuto (vedere come leggere la roba dalla shared memory). Una volta finita la lettura si effettuerà il deattach
-Il processo master alla fine farà il deattach finale così da garantire la rimozione della shared memory
(L'ho scritto come promemoria per non dimenticarmelo)*/
#define MSGQ_KEY "12345"
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}

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
    char *prova;
    char *msgq_key[]={MSGQ_KEY,NULL};
    struct msg_buf buf;
    struct msqid_ds data;
    

    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    buf.type=1;
    buf.mtext=malloc(sizeof(int)*12);
    buf.mtext=macros;

    key=msgget(atoi(MSGQ_KEY),IPC_CREAT | 0660);
    printf("L'id della coda di messaggi è:%d\n",key);
    err=msgsnd(key,&buf,sizeof(int)*12,0);
    TEST_ERROR

    for(z=0;z<N_MACRO;z++){
        printf("Macro %d°:%d\n",z+1,buf.mtext[z]);
    }
    for(i=0;i<N_USERS;i++){
        switch(fork()){
            /*Child code*/
            case 0:
            /*execveing  user processes*/
            /*execve("User",MSGQ_KEY,NULL);*/
            err=execve("User",msgq_key,NULL);
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


    return 0;
}
/*Function used to read macros from "macros.txt". They are then saved in macros*/
/*TODO: FIXARE LA ROBA DEGLI SPAZI IN MACROS.TXT*/
void read_macros(int fd,int * macros){
    int j=0,i,tmp;
    char c;
    char *string=malloc(6);/*Macros' parametere cant have more than 5/6 digits*/
    while(read(fd,&c,1) != 0){
        if(c == ':'){ 
            read(fd,&c,1);
            for( i=0;c>=48 && c<=57;i++){
                *(string+i)=c;
                read(fd,&c,1);
            }
             tmp = atoi(string);
            macros[j]=tmp;
            j++;
        }
    }
}

