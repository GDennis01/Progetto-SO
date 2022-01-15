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
    /*Attach to shm*/
    shm_info=shmat(*info_key,NULL,0660);
    shm_macro=shmat(*macro_key,NULL,0660);
    mastro_area_memoria = (transaction_block*)shmat(*mastro_key, NULL, 0);  /*così leggi a blocchi di transazione*/
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
int compareBudget(const void * a, const void * b){
    int aa = (int)(((budgetSortedArray *)a)->budget);
    int bb = (int)(((budgetSortedArray *)b)->budget);
    return aa-bb;
}
void check_err_keys(int info_key,int macro_key,int sem_key,int mastro_key){
 
    if(info_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(macro_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(sem_key==-1){
        TEST_ERROR
        exit(1);
    }
    if(mastro_key==-1){
        TEST_ERROR
        exit(1);
    }
}
/*Method that initialize the 4 semaphores*/
void initSem(int sem_id,int n_users,int n_nodes){
    semctl(sem_id,0,SETVAL,n_users+n_nodes);/*Semaphore used to synchronize writer(master) and readers(nodes/users)*/
    semctl(sem_id,1,SETVAL,n_nodes);/*Semaphore used to allow nodes to finish creating their queues*/
    semctl(sem_id,2,SETVAL,1);/*Semaphore used to synchronize nodes writing on mastro*/
    semctl(sem_id,3,SETVAL,1);/*Semaphore used by master to wait for nodes to finish creating their queues*/
}

int getBudget(int my_index){
    return shm_info[my_index].budget;
}
