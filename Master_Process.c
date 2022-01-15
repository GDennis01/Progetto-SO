#include "common.c"
#define MAX_PRINTABLE_PROCESSES 10
 /* Per poter compilare con -std=c89 -pedantic */
/*
GESTIONE NODI:
Problema: Il master può creare N_NODES + X dove X varia con il crescere del tempo.
Dove salvo questi X nodi? Dovrei avere una shared memory con una dimensione che varia nel tempo -> Impossibile

Soluzione pensata: allocare uno spazio extra nella shared memory. Questa servirà per salvare temporaneamente
il nuovo nodo creato. Il master, gli user e i nodi se lo salveranno localmente ogni volta che questo viene generato

Problema 2: I nuovi nodi creati come fanno a tener traccia degli X-1 nodi creati prima di lui? 
Solo i nodi "originali" riescono ad aver traccia di TUTTI i nodi creati

Soluzione pensata: L'X-esimo nodo creato, per salvarsi gli X-1 nodi creati prima di lui, leggerà da
una coda di messaggi, il messaggio con tipo=getpid() contenente tutti i pid degli X-1 nodi creati prima di lui.
Se ne occuperà il master di inviare un messaggio, di tipo getpid()(dove il pid è quello del nuovo nodo creato)
contenente tutti i nuovi nodi creati prima di lui.

P.S. : Per nuovi nodi creati, si intendono quelli generati quando la transaction pool di ogni nodo "originale" è piena
*/
/*
    Ultime modifiche
        -03/01/2022
            -Modifica al metodo initIPCs()
            -Ora ci sono solo tre key: info,macro e sem
            -Le macro vengono salvate in *shm_macro, le info dei processi in *info
            -alarmHandler -> signalsHandler
            -Aggiunto mastro_key e la sua relativa shared
*/
void printTransaction(struct transaction tr){
    printf("[MASTER PROCESS]Sender:%d\n",tr.sender);
    printf("[MASTER PROCESS]Receiver:%d\n",tr.receiver);
    printf("[MASTER PROCESS]Reward:%d\n",tr.reward);
    printf("[MASTER PROCESS]Timestamp Sec:%ld  NSec:%ld\n",tr.timestamp.tv_sec,tr.timestamp.tv_nsec);
    printf("[MASTER PROCESS]Amount:%d\n",tr.amount);
}
int dims=0;
int info_key,macro_key,sem_key,mastro_key;/*key:key for shared memory key2:key for semaphores  key3:key for message queue*/

int compareBudget(const void * a, const void * b){
    int aa = (int)(((budgetSortedArray *)a)->budget);
    int bb = (int)(((budgetSortedArray *)b)->budget);
    return aa-bb;
}

int main(int argc, char const *argv[])
{
    struct sigaction sa;
    int  i,j,z,status,err,child_pid,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    int flag;
    char str[15],str2[15],str3[15],str4[15];/*Stringified key for shm*/
    char *arguments[]={NULL,NULL,NULL,NULL,NULL,NULL};/*First arg of argv should be the filename by default*/
    struct sembuf sops;
    int activeProcess= 0;
    budgetSortedArray *budgetArray;
    budgetSortedArray *budgetArrayNO;
    int *friends;
    int pid_friend;
    msgqbuf node_friend;

    struct timespec time;
    time.tv_sec=1;
    time.tv_nsec=0;

    srand(getpid());

    bzero(&sa,sizeof(sa));
    sa.sa_handler=signalsHandler;
    sigaction(SIGALRM,&sa,NULL);
    sigaction(SIGUSR1,&sa,NULL);

    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macross*/

    budgetArray = (budgetSortedArray *)malloc(sizeof(budgetSortedArray)*N_USERS);
    budgetArrayNO = (budgetSortedArray *)malloc(sizeof(budgetSortedArray)*N_NODES);
    dims=sizeof(info_process)*(N_USERS+N_NODES);

    initIPCS(&info_key,&macro_key,&sem_key,&mastro_key, dims);
    shm_info=shmat(info_key,NULL,0660);
    shm_macro=shmat(macro_key,NULL,0660);
    mastro_area_memoria = (transaction_block*)shmat(mastro_key, NULL, 0);  /*così leggi a blocchi di transazione*/


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
    /*Initializing the first area of the shared memory of the ledger*/
    mastro_area_memoria[0].executed=0;
    
    TEST_ERROR
    
    printf("[PARENT #%d] ID della SHM_INFO:%d     ID del SEM:%d\n",getpid(),info_key,sem_key);
    /*TODO:inglobare sti sprintf in una unica funzione*/
    sprintf(str,"%d",info_key);/*I convert the key from int to string*/
    arguments[1]=str;
    
    sprintf(str2,"%d",macro_key);/*I convert the key from int to string*/
    arguments[2]=str2;/*Had to create another temporary string(str2) due to strange behaviour with sprintf*/
    
    sprintf(str3,"%d",sem_key);/*I convert the key from int to string*/
    arguments[3]=str3;

    sprintf(str4,"%d",mastro_key);/*I convert the key from int to string*/
    arguments[4]=str4;

    /*Writing Macros to shared memory*/
    for(i=0;i<N_MACRO;i++){
        shm_macro[i]=macros[i];
    }

    /*Semaphores initialization*/
    semctl(sem_key,0,SETVAL,N_USERS+N_NODES);/*Semaphore used to synchronize writer(master) and readers(nodes/users)*/
    semctl(sem_key,1,SETVAL,N_NODES);/*Semaphore used to allow nodes to finish creating their queues*/
    semctl(sem_key,2,SETVAL,1);/*Semaphore used to synchronize nodes writing on mastro*/
    semctl(sem_key,3,SETVAL,1);/*Semaphore used by master to wait for nodes to finish creating their queues*/

    /*Triggering the countdown of the simulation*/
    alarm(SO_SIM_SEC);

    for(i=0;i<N_USERS;i++){
        switch(child_pid=fork()){
            /*To avoid inconsistency reading from user processes, I stop said processes at a semaphore till the father is done writing PIDs in the shm array*/
            case 0:
                sops.sem_num=0;
                sops.sem_op=0;
                sops.sem_flg=0;

                semop(sem_key,&sops,1);/*wait for 0 operation*/
                arguments[0]="User";
                execve("User",arguments,NULL);
                exit(1);
            break;

            /*Parent code*/
            default:

                shm_info[i].pid=child_pid;
                shm_info[i].type=0;
                shm_info[i].budget=SO_BUDGET_INIT;
                shm_info[i].alive=1;
                
                sops.sem_num=0;
                sops.sem_op=-1;
                sops.sem_flg=0;
                semop(sem_key,&sops,1);/*decreasing the value by 1*/
            break;
        }
    }

    /*Generating children node processes*/
    /*semctl(sem_key,0,SETVAL,N_NODES);*/
    for(i=0;i<N_NODES;i++){
        switch(child_pid=fork()){
            /*Child code*/
            case 0:
            sops.sem_num=0;
            sops.sem_op=0;
            sops.sem_flg=0;
            semop(sem_key,&sops,1);/*wait for 0 operation*/
            
            arguments[0]="Node";
            execve("Node",arguments,NULL);
            exit(1);
            break;

            /*Parent code*/
            default:

            shm_info[N_USERS+i].pid=child_pid;
            shm_info[N_USERS+i].type=1;
            shm_info[N_USERS+i].alive=1;
            shm_info[N_USERS+i].budget=0;

            
            sops.sem_num=0;
            sops.sem_op=-1;
            sops.sem_flg=0;
            semop(sem_key,&sops,1);/*decreasing the value by 1*/
            break;
        }
    }

    /*TODO:rendere sto aborto un metodo così non si nota*/
    /*Aspetto che i nodi finiscano di creare le loro code*/
    sops.sem_num=1;
    sops.sem_op=0;
    sops.sem_flg=0;
    semop(sem_key,&sops,1);

    j=0;
    while(j<N_NODES){
      
        i=0;
        friends=malloc(sizeof(info_process)*SO_N_FRIENDS);/*Creating tmp array to store all friends that are going to be sent*/
        while(i<SO_N_FRIENDS){
            flag=1;
            do{
                pid_friend=rand()%N_NODES;
                flag=1;
                
                if(pid_friend != j){/*A node cannot be friend to himself. The index identifies univoquely the node thus I compare them*/
                for(z=0;z<i && flag==1;z++){
                    if(pid_friend == friends[z]){
                        flag=0;
                    }
                }
                }else flag =0;
               
            }while(flag==0);

            friends[i]=pid_friend;
            i++;
        }

        /*printf("Amici del nodo %d:\n",j);
        for(z=0;z<SO_N_FRIENDS;z++){
            printf("Amico %d°:%d\n",j,friends[z]);
        }*/
        for(z=0;z<SO_N_FRIENDS;z++){
            /*Sending friends one by one in form of a fake transaction*/
            node_friend.mtype=shm_info[N_USERS+j].pid;
            node_friend.tr.receiver=friends[z];
            if(msgsnd(msgget(shm_info[N_USERS+j].pid,0666),&node_friend,sizeof(node_friend.tr),0) == -1 ){
                TEST_ERROR
            }
        }
        j++;
    }
    /*Rilascio il semaforo così i nodi/user possono chillare*/
    sops.sem_num=3;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sem_key,&sops,1);

    /*Waiting for all children to DIE*/
    while(1){
        nanosleep(&time,NULL);  /*1 sec */
        printf("-----STAMPAUSERS-----\n");
        /*calcolo child attivi */
        for(i=0;i<N_USERS;i++){
            if(shm_info[i].alive==1){
                budgetArray[activeProcess].budget = shm_info[i].budget;
                budgetArray[activeProcess].index = i;
                activeProcess++;
            }
        }
        printf("Utenti ancora attivi: %d / %d \n",activeProcess,N_USERS);
        budgetArray = realloc(budgetArray, sizeof(budgetSortedArray)*activeProcess);
        if(activeProcess == 0){
            terminazione(2, dims);  /*all dead we close this shit*/
        }else if(activeProcess > MAX_PRINTABLE_PROCESSES){   /*if child > x -> stampa solo di quelli con più budget e quelli con meno budget */
            qsort(budgetArray, activeProcess, sizeof(budgetSortedArray), compareBudget);
            for(i=0; i<MAX_PRINTABLE_PROCESSES/2; i++){
                printf("[User Process #%d]\nBilancio:%d\n",shm_info[budgetArray[activeProcess-i].index].pid, budgetArray[activeProcess-i].budget);
            }
            printf("...\n");
            for(i=MAX_PRINTABLE_PROCESSES/2; i>=0; i--){
                printf("[User Process #%d]\nBilancio:%d\n",shm_info[budgetArray[i].index].pid,budgetArray[i].budget);
            }
        }else{
            for(i=0;i<activeProcess;i++){
                printf("[User Process #%d]\nBilancio:%d\n",shm_info[budgetArray[i].index].pid,budgetArray[i].budget);
            }
        }
        printf("-----STAMPANODI-----\n");

        for(i=N_USERS;i<N_USERS+N_NODES;i++){
            budgetArrayNO[i-N_USERS].budget = shm_info[i].budget;
            budgetArrayNO[i-N_USERS].index = i;
        }
        if(N_NODES>MAX_PRINTABLE_PROCESSES){
            qsort(budgetArrayNO, N_NODES, sizeof(budgetSortedArray), compareBudget);
            for(i=0; i<MAX_PRINTABLE_PROCESSES/2; i++){
                printf("[Node Process #%d]\nBilancio:%d\n",shm_info[budgetArrayNO[N_NODES-i].index].pid, budgetArrayNO[N_NODES-i].budget);
            }
            printf("...\n");
            for(i=MAX_PRINTABLE_PROCESSES/2; i>=0; i--){
                printf("[Node Process #%d]\nBilancio:%d\n",shm_info[budgetArrayNO[i].index].pid,budgetArrayNO[i].budget);
            }
        }else{
            for(i=N_USERS;i<N_USERS+N_NODES;i++){
                printf("[Node Process #%d]\nBilancio:%d\n",shm_info[budgetArrayNO[i-N_USERS].index].pid,budgetArrayNO[i-N_USERS].budget);
            }
        }
        
        activeProcess=0;
    }
    printf("[PARENT] ABOUT TO ABORT\n");
    
    return 0;
}

void terminazione(int reason,int dim){
    int i=0,j=0,cnt=0;
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME,&curr_time);
    printf("TIMESTAMP STAMPA:sec:%ld nsec:%ld\n",curr_time.tv_sec,curr_time.tv_nsec);
    printf("-----------------------------------------------\n");
    printf("Il motivo della terminazione è: %s \n",reason==0?"E' scaduto il tempo della simulazione":reason ==1?"La capacità del libro mastro si è esaurita":reason ==2?"Tutti i processi utenti sono terminati":"Motivo della terminazione ignoto. Errore");
    for(i=0;i<dim/sizeof(info_process);i++){
        kill( shm_info[i].pid , SIGTERM ); /*Killing every children*/
        waitpid(shm_info[i].pid,NULL,0);
    }
    for(i=0;i<dim/sizeof(info_process);i++){
        if(shm_info[i].type==0){
            printf("------------------------------------\n");
            printf("[User Process #%d]\nBilancio:%d\nTerminato prematuramente:%s\n",shm_info[i].pid,shm_info[i].budget,shm_info[i].abort_trans==1?"Sì":"No");
            printf("------------------------------------\n");

            if(shm_info[i].abort_trans==1)
                cnt++;
                
            /*kill( shm_info[i].pid , SIGTERM ); 
            waitpid(shm_info[i].pid,NULL,0);*/
        }else if(shm_info[i].type==1){
                        printf("------------------------------------\n");

            printf("[Node Process #%d ind:%d]\nBilancio:%d\nTransazioni rimanenti nella transaction pool:%d\n",shm_info[i].pid,i,shm_info[i].budget,shm_info[i].abort_trans);
                        printf("------------------------------------\n");

            /*kill( shm_info[i].pid , SIGTERM ); 
            
            waitpid(shm_info[i].pid,NULL,0);*/
        }else printf("Errore, processo sconociuto\n");
         
    }
    
    printf("Numero di Processi Utenti terminati prematuramente:%d\n",cnt);

    printf("Stampa libro mastro:\n");
    for(i=0;i<SO_REGISTRY_SIZE;i++){
        if(mastro_area_memoria[i].executed==0){
            printf("[MASTER PROCESS] Il libro mastro ha ancora %d blocchi liberi!\n",SO_REGISTRY_SIZE-i);
            break;
        }
        printf("[MASTER PROCESS] Blocco %d°:\n",i+1);
        for(j=0;j<SO_BLOCK_SIZE;j++){
            printf("------------------------------\n");
            printf("[MASTER PROCESS]Transazione %d°:\n",j+1);
            printTransaction(mastro_area_memoria[i].transactions[j]);
            printf("------------------------------\n");

        }

    }
    deleteIPCs(info_key,macro_key,sem_key,mastro_key);
    exit(EXIT_SUCCESS);
}

/*
    FIXME:fixare il bug dove se non c'è un \n alla fine crasha
*/
void read_macros(int fd,int * macros){
    int j=0,i,tmp;
    char c;
    char *string=malloc(13);/*Macros' parameter cant have more than 13 digits*/
    while(read(fd,&c,1) != 0){
        if(c == ':'){ 
            read(fd,&c,1);
            for( i=0;c>=48 && c<=57;i++){
                *(string+i)=c;
                read(fd,&c,1);
            }
            tmp = atoi(string);
            bzero(string,13);/*Erasing the temporary string since it may cause data inconsistency*/
            macros[j]=tmp;
            j++;
        }
    }
}

void signalsHandler(int signal) {
    switch(signal){
    case SIGALRM:
        terminazione(0,dims);
        break;
    case SIGUSR1: /*Libro mastro is full*/
        terminazione(1,dims);
        break;
    default:
        printf("Bruh\n");
    }
}
