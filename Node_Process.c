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
 int main(int argc, char const *argv[])
{
    int id,i;
    
    transaction tr;
    int budget;
    
    id=atoi(argv[1]);/*shared memory id to access shared memory with macros*/
    shm_buf=(child*)shmat(id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    printf("[NODE CHILD #%d] ID della SHM:%d\n",getpid(),id);
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_buf[i].pid;
    }
    shmdt(&shm_buf);
    printf("[NODE CHILD] ABOUT TO ABORT\n");

    return 0;
}

