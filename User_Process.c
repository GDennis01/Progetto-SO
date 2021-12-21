#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "User_Process.h"
#include "common.c"

#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}
int main(int argc, char const *argv[])
{
    int id,key,i;
    struct shm_buf *buf;
    /*shared memory key to shared memory with macros*/
    key=atoi(argv[0]);

    TEST_ERROR
  
     id=shmget(key,sizeof(buf->mtext),IPC_CREAT);
    buf=shmat(id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/
      printf("ID della SHM:%d\n",id);
    TEST_ERROR
    
        for(i=0;i<12;i++){
            printf("Macro %d°:%d\n",i+1,buf->mtext[i]);
        }
        shmdt(buf);
        
    

    return 0;
}
