
#include "common.h"


void read_macros(int fd,int * macros){
    int j=0,i,tmp;
    char c;
    char *string=malloc(10);/*Macros' parameter cant have more than 10 digits*/
    while(read(fd,&c,1) != 0){
        if(c == ':'){ 
            read(fd,&c,1);
            for( i=0;c>=48 && c<=57;i++){
                *(string+i)=c;
                read(fd,&c,1);
            }
            tmp = atoi(string);
            bzero(string,10);/*Erasing the temporary string since it may cause data inconsistency*/
            macros[j]=tmp;
            j++;
        }
    }
}
    /*Method to initialize IPCs objects*/
void initIPCS(int *shm_key,int *sem_key,int *msgq_key,int dim){
    *shm_key=shmget(IPC_PRIVATE,dim,IPC_CREAT| 0660);/*Getting the key for the shared memory(and also initializing it)*/
    *sem_key=semget(IPC_PRIVATE,1,IPC_CREAT | 0660);
    *msgq_key=msgget(IPC_PRIVATE,IPC_CREAT | 0660);
}
void deleteIPCs(int shm_key,int sem_key,int msgq_key){
    shmctl(shm_key,IPC_RMID,NULL);/*Detachment of shared memory*/
    semctl(sem_key,0,IPC_RMID,NULL);/*Deleting semaphore*/
    msgctl(msgq_key,IPC_RMID,NULL);
}
