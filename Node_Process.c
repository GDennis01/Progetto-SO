#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "Node_Process.h"
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

 int main(int argc, char const *argv[])
{
    int key;
    int msgId;
    printf("BELLA REGA SON NATO :(((\n");
    printf("La SHM_KEY E':%s\n",argv[0]);/*arv[0] is the shmemory key*/
    
    msgId = atoi(argv[3]);
    
    malloc(SO_TP_SIZE * sizeof(transaction));
    /* creo area di memoria di dimensione 1 blocco di transazione*/
    /*ora leggo e ce ficco dentro le cose dalla msg queue */
    msgrcv ( msgId , void * msgp , sizeof(transaction) , 1, 0 ); /* 0 oppure null per flag ? ed il buffer ? */
    /* ora è nel buffer che dovevo dichiarare e da li me lo leggo per la reward e salvarlo nella mia memoria malloccata prima */
    
    return 0;
}

