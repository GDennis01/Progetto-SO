
#include "common.h"
struct transaction creaTransazione(pid_t rec, unsigned int budget,int N_USERS,int PERCENTUALE_REWARD){
	unsigned int seed = CLOCK_REALTIME;
	struct transaction tr;
	int amount = getRand(1,budget);
    struct child arrayChildren[N_USERS];

	tr.timestamp = getTime();
	tr.sender = getPid();
	tr.receiver = arrayChildren[(int)getRand(0,N_USERS - 1)].pid;
	tr.reward = (int)(PERCENTUALE_REWARD * (float)amount);
	tr.amount = amount - tr.reward;
	return tr;
}
void read_macros(int fd,int * macros){
    int j=0,i,tmp;
    char c;
    char *string=malloc(6);/*Macros' parametere cant have more than 5/6 digits*/
    while(read(fd,&c,1) != 0){
        if(c == ':'){ 
            read(fd,&c,1);
            for( i=0;c>=48 && c<=57;i++){
                *(string+i)=c;
                read(fd,&c,1);
            }
             tmp = atoi(string);
            macros[j]=tmp;
            j++;
        }
    }
}