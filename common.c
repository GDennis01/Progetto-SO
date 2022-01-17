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
    msgctl(msgget(getpid(),0666),IPC_RMID,NULL);
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

trans_pool insertTail(trans_pool p,transaction tr){

    trans_pool new_el,tmp;
	if(p == NULL){
		printf("The list is empty, gonna initialize it\n");
		return p;
	}
	/*
	Alloco sizeof(*new_el) bytes per il nuovo nodo.
	Non alloco sizeof(list) perchè list è un puntatore.
	Quindi per la size giusta gli passo la lista deferenziata
	*/
	new_el=malloc(sizeof(*new_el));
    
    new_el->amount=tr.amount;
    new_el->executed=tr.executed;
    new_el->hops=tr.hops;
    new_el->receiver=tr.receiver;
    new_el->reward=tr.reward;
    new_el->sender=tr.sender;
    new_el->timestamp=tr.timestamp;
	new_el->next=NULL;
	for( tmp=p;tmp->next!=NULL;tmp=tmp->next){
	}
	tmp->next=new_el;
    return p;

}

trans_pool insertHead(trans_pool p,transaction tr){

    trans_pool new_el;

	/* Allocate the new node */
	new_el = malloc(sizeof(*new_el));
	/* Setting the data */
	
    new_el->amount=tr.amount;
    new_el->executed=tr.executed;
    new_el->hops=tr.hops;
    new_el->receiver=tr.receiver;
    new_el->reward=tr.reward;
    new_el->sender=tr.sender;
    new_el->timestamp=tr.timestamp;
	/* In this implementation, the next is set to NULL. If this method gets invokated, it means the list is empty, thus the next will be NULL */
	new_el->next = NULL;
	/* new head is the pointer to the new node */
	return new_el;
}

void transFree(trans_pool p){
    /* If p ==  NULL, nothing to deallocate */
	if (p == NULL) {
		return;
	}
	/* First deallocate (recursively) the next nodes... */
	transFree(p->next);
	/* ... then deallocate the node itself */
	free(p);
}

void transPrint(trans_pool p){
    trans_pool tmp=p;
    if (p == NULL) {
		printf("Empty list\n");
		return;
	}
	printf("Sender:%d\n",p->sender);
    printf("Receiver:%d\n",p->receiver);
    printf("Reward:%d\n",p->reward);
    printf("Timestamp Sec:%ld  NSec:%ld\n",p->timestamp.tv_sec,p->timestamp.tv_nsec);
    printf("Amount:%d\n",p->amount);
	for(; tmp->next!=NULL; tmp = tmp->next) {
	printf("Sender:%d\n",tmp->sender);
    printf("Receiver:%d\n",tmp->receiver);
    printf("Reward:%d\n",tmp->reward);
    printf("Timestamp Sec:%ld  NSec:%ld\n",tmp->timestamp.tv_sec,tmp->timestamp.tv_nsec);
    printf("Amount:%d\n",tmp->amount);
	}
	printf("\n");
}

trans_pool transDeleteIf(trans_pool head , transaction tr){
 trans_pool tmp=head;
	trans_pool prev=NULL;
    if(head == NULL)
        return head;
	/*se l'elemento da togliere è il primo, restituisco la lista a partire dal secondo elemento*/
	if(head->sender == tr.sender && head->receiver == tr.receiver && (head->timestamp.tv_nsec == tr.timestamp.tv_nsec && head->timestamp.tv_sec == tr.timestamp.tv_sec)){
		tmp=tmp->next;/*aggiorno il tmp così posso deallocare head*/
		free(head);
		return tmp;
	}
	while(tmp != NULL){
			if(head->sender == tr.sender && head->receiver == tr.receiver && (head->timestamp.tv_nsec == tr.timestamp.tv_nsec && head->timestamp.tv_sec == tr.timestamp.tv_sec)){
			prev->next=tmp->next;
			free(tmp);
			return head;
		}
		prev=tmp;
		tmp=tmp->next;
	}
	return head;
}

int countTrans(trans_pool p){
    int n=0;
    trans_pool tmp = p;
    printf("PROVA\n");
    while(tmp->next!=NULL){
        tmp->next;
        n++;
    }
    printf("CI SONO %d TRANSAZIONI\n",n);
    return n;
}

