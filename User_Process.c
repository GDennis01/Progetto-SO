#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "User_Process.h"
#include "Master_Process.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}
int main(int argc, char const *argv[])
{
    int id,key,nbyte,i;
    int *macross=malloc(12*sizeof(int));/*12 is the number of macros defined in macros.txt*/
    struct msg_buf buf;
    
    printf("La MSGQ_KEY E':%s\n",argv[0]);/*arv[0] is the msg_q key*/
    key=atoi(argv[0]);
    id=msgget(key,IPC_CREAT | 0600);
    TEST_ERROR
    printf("L'id della coda di messaggi è :%d\n",id);
    nbyte=msgrcv(id,&buf,sizeof(int)*12,1,MSG_COPY | IPC_NOWAIT );
    if(nbyte == 0){
        printf("Errore bro");
    }else{
        printf("Letti %d nbyte\n",nbyte);
        TEST_ERROR
    }
        for(i=0;i<12;i++){
            /*printf("Macro %d°:%d\n",i,buf.macros[i]);*/
        }
    

    return 0;
}
