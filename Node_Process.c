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

struct transaction creaTransazioneRicompensa(int sommaReward){	/* da mettere in common */
    transaction tr;
    struct timespec curr_time;
    
    clock_gettime(CLOCK_REALTIME,&curr_time);
    srand(curr_time.tv_nsec);/*initializing RNG seed with the current timestamp(?)*/

    tr.receiver = getpid();
    tr.sender = -1;    

	tr.timestamp = curr_time.tv_nsec;/*current clock_time*/
	tr.amount = sommaReward;
	
    tr.reward = 0;
    return tr;
}

 int main(int argc, char const *argv[])
{
    int key;
    int msgId;
    int numeroTransazioniMemorizzate = 0, sommaReward = 0;
    struct message *msg_buf;
    struct transaction * memoriaTransazioni;
    printf("BELLA REGA SON NATO :(((\n");
    printf("La SHM_KEY E':%s\n",argv[0]);/*arv[0] is the shmemory key*/
    
    msgId = atoi(argv[3]);
    
    memoriaTransazioni = (transaction*)malloc(SO_TP_SIZE * sizeof(transaction));
    msg_buf = (message *)malloc(sizeof(message);
    /* creo area di memoria di dimensione 1 blocco di transazione*/
    /*ora leggo e ce ficco dentro le cose dalla msg queue */
    
    /* ora è nel buffer che dovevo dichiarare e da li me lo leggo per la reward e salvarlo nella mia memoria malloccata prima */
    while(1){
    	if(numeroTransazioniMemorizzate != SO_TP_SIZE-1){
    		msgrcv ( msgId , &msg_buf , sizeof(transaction) , 1, 0 ); /* 0 oppure null per flag ? ed il buffer ? */
    		TEST_ERROR
    		memoriaTransazioni[numeroTransazioniMemorizzate] = msg_buf.tr;
    		sommaReward += (msg_buf.tr).reward;	/* dubbia */
    		numeroTransazioniMemorizzate++;
    	}else{
    		memoriaTransazioni[SO_TP_SIZE-1] = creaTransazioneRicompensa(sommaReward);
    		numeroTransazioniMemorizzate = 0;
    		sommaReward = 0;
    		/*incollare tutto ciò nel libro mastro usando un semaforo */
    	}
    }
    
    return 0;
}

