#include "common.h"

/*
    Ultime modifiche
        -05/01/2022
            -Ora deleteIPCs() rimuove correttamente tutte le memorie condivise
            -Spostato getBudget() da User.c a common.c


*/


/*Method to initialize IPCs objects
TODO:Spostarlo in master.c maybe?*/
void initIPCS(int *info_key,int *macro_key,int *sem_key, int *mastro_key, int dims){
    *info_key=shmget(IPC_PRIVATE,dims,IPC_CREAT| 0660);/*Getting the key for the shared memory(and also initializing it)*/
    *macro_key=shmget(IPC_PRIVATE,N_MACRO*sizeof(int),IPC_CREAT | 0660);
    *sem_key=semget(IPC_PRIVATE,4,IPC_CREAT | 0660);
    *mastro_key = shmget(IPC_PRIVATE,SO_REGISTRY_SIZE*sizeof(transaction_block),IPC_CREAT| 0660);
}

void deleteIPCs(int info_key,int macro_key,int sem_key, int mastro_key){
    shmdt(shm_info);
    shmdt(shm_macro);
    shmdt(mastro_area_memoria);
    shmctl(info_key,IPC_RMID,NULL);/*Detachment of shared memory*/
    shmctl(macro_key,IPC_RMID,NULL);/*Detachment of shared memory*/
    semctl(sem_key,0,IPC_RMID,NULL);/*Deleting semaphore*/
    shmctl(mastro_key,IPC_RMID,NULL);
    /*msgctl(msgq_key,IPC_RMID,NULL);*/
}

int getBudget(int my_index){
    return shm_info[my_index].budget;
}
