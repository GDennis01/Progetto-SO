
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
void initIPCS(int *info_key,int *macro_key,int *sem_key, int *mastro_key, int dims){
    *info_key=shmget(IPC_PRIVATE,dims,IPC_CREAT| 0660);/*Getting the key for the shared memory(and also initializing it)*/
    *macro_key=shmget(IPC_PRIVATE,N_MACRO*sizeof(int),IPC_CREAT | 0660);
    *sem_key=semget(IPC_PRIVATE,2,IPC_CREAT | 0660);
    *mastro_key = shmget(IPC_PRIVATE,SO_REGISTRY_SIZE*SO_BLOCK_SIZE*sizeof(transaction),IPC_CREAT| 0660);
}

void deleteIPCs(int info_key,int macro_key,int sem_key, int mastro_key){
    shmctl(info_key,IPC_RMID,NULL);/*Detachment of shared memory*/
    shmctl(macro_key,IPC_RMID,NULL);/*Detachment of shared memory*/
    semctl(sem_key,0,IPC_RMID,NULL);/*Deleting semaphore*/
    shmdt(mastro_area_memoria);
    /*msgctl(msgq_key,IPC_RMID,NULL);*/
}
