#include "common.c"

int macros[N_MACRO];
child *pid_users;
child *pid_nodes;
 int main(int argc, char const *argv[])
{
    int shm_id,msgq_key,sem_id,i,j;
    int budget;
    int bytes_read;
    transaction tr;
    msgqbuf msg_buf;
    info_process infos;
    struct sembuf sops;

    /*Initializing infos that will be sent every second to the master process*/
    infos.pid=getpid();
    infos.type=1;
    infos.budget=0;
    infos.abort_trans=0;
    
    
    
    shm_id=atoi(argv[1]);/*shared memory id to access shared memory with macros*/
    shm_buf=(child*)shmat(shm_id,NULL,SHM_RDONLY);/*Attaching to shm with macros*/

    sem_id=atoi(argv[3]);
    /*msgq_key=atoi(argv[2]);*/
    msgq_key=msgget(getpid(),IPC_CREAT | 0660);
    /*The semaphore is used so that all nodes can create their queues without generating inconsistency*/
    sops.sem_num=1;
    sops.sem_op=-1;
    sops.sem_flg=0;
    semop(sem_id,&sops,1);
    printf("[NODE CHILD #%d] SEMVAL:%d\n",getpid(),semctl(sem_id,1,GETVAL));
    printf("[NODE CHILD #%d] ID della SHM:%d\n",getpid(),shm_id);
    /*Storing macros in a local variable. That way I can use macros defined in common.h*/
    for(i=0;i<N_MACRO;i++){
        macros[i]=shm_buf[i].pid;
    }
    /*Populating the pid user array*/
    pid_users=malloc(sizeof(child)*N_USERS);
    for(j=0,i=N_MACRO;i<N_MACRO+N_USERS;i++,j++){
        pid_users[j].pid=shm_buf[i].pid;
        pid_users[j].status=shm_buf[i].status;   
    }
    /*Populating the pid nodes array*/
    pid_nodes=malloc(sizeof(child)*N_NODES);
    for(j=0,i=N_MACRO+N_USERS;i<N_MACRO+N_USERS+N_NODES;i++,j++){
        pid_nodes[j].pid=shm_buf[i].pid;
        pid_nodes[j].status=shm_buf[i].status;  
    }

    bytes_read=msgrcv(msgq_key,&msg_buf,sizeof(msg_buf.tr),getpid(),0);
    printf("[NODE CHILD #%d] LETTI %d BYTES\n",getpid(),bytes_read);
    shmdt(&shm_buf);
    printf("[NODE CHILD] ABOUT TO ABORT\n");
    msgctl(msgq_key,IPC_RMID,NULL);

    return 0;
}

void updateInfos(int budget,int abort_trans,info_process* infos){
    infos->budget=budget;
    infos->abort_trans=abort_trans;
}

