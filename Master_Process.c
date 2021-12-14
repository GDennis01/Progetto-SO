#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#include "Master_Process.h"
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

#define TEST_ERROR if (errno) {fprintf(stderr,				\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}

#define N_MACRO 12 /*Numbers of macros to read from "macros.txt"*/
/*Macros*/
#define N_USERS macros[0]
#define N_NODES macros[1]
#define MIN_TRANS_GEN_NSEC macros[2]
#define MAX_TRANS_GEN_NSEC macros[3]
#define MIN_TRANS_PROC_NSEC macros[4]
#define MAX_TRANS_PROC_NSEC macros[5]
#define SO_BUDGET_INIT macros[6]
#define SO_REWARD macros[7]
#define SO_RETRY macros[8]
#define SO_TP_SIZE macros[9]
#define SO_N_FRIENDS macros[10]
#define SO_SIM_SEC macros[11]

int main(int argc, char const *argv[])
{
    int  i,fd = open("macros.txt",O_RDONLY);
    int macros[N_MACRO];
    read_macros(fd,macros);/*I read macros from file*/
    close(fd);/*I close the fd used to read macros*/

    for(i=0;i<N_USERS;i++){
        switch(fork()){
            /*Child code*/
            case 0:
            /*execveing  user processes*/
            break;

            /*Parent code*/
            default:
            /*handling stuff*/
            break;
        }
    }

    for(i=0;i<N_NODES;i++){
        switch(fork()){
            /*Child code*/
            case 0:
            /*execveing  node processes*/
            break;

            /*Parent code*/
            default:
            /*handling stuff*/
            break;
        }
    }


    return 0;
}
/*Function used to read macros from "macros.txt". They are then saved in macros*/
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