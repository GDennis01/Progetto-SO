#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "common.c"
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
int macros[N_MACRO];
child *pid_users;
child *pid_nodes;
 int main(int argc, char const *argv[])
{
    int shm_id,msgq_key,i,j;
    int budget;
    transaction tr;
    msgqbuf msg_buf;
    
    
    
    shm_id=atoi(argv[1]);/*shared memory id to access shared memory with macros*/
    shm_buf=(child*)shmat(shm_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    msgq_key=atoi(argv[2]);

    printf("[NODE CHILD #%d] ID della SHM:%d\n",getpid(),shm_id);
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_buf[i].pid;
    }

    pid_users=malloc(sizeof(child)*N_USERS);
    for(j=0,i=N_MACRO;i<N_MACRO+N_USERS;i++,j++){
        pid_users[j].pid=shm_buf[i].pid;
        pid_users[j].status=shm_buf[i].status;   
    }

    pid_nodes=malloc(sizeof(child)*N_NODES);
    for(j=0,i=N_MACRO+N_USERS;i<N_MACRO+N_USERS+N_NODES;i++,j++){
        pid_nodes[j].pid=shm_buf[i].pid;
        pid_nodes[j].status=shm_buf[i].status;  
    }

    msgrcv(msgq_key,&msg_buf,sizeof(msg_buf.tr),getpid(),0);
    shmdt(&shm_buf);
    printf("[NODE CHILD] ABOUT TO ABORT\n");

    return 0;
}

