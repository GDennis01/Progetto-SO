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
    struct msg_buf *buf;
    
    key=atoi(argv[0]);
    /*id=msgget(key,IPC_CREAT | 0600);*/
    TEST_ERROR
  
     id=shmget(key,sizeof(buf->mtext),IPC_CREAT);
    buf=shmat(id,NULL,SHM_RDONLY);
      printf("ID della SHM:%d\n",id);
    TEST_ERROR
    
        for(i=0;i<12;i++){
            printf("Macro %d°:%d\n",i+1,buf->mtext[i]);
        }
        shmdt(buf);
        
    

    return 0;
}
