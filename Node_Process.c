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
    printf("BELLA REGA SON NATO :(((\n");
    printf("La SHM_KEY E':%s\n",argv[0]);/*arv[0] is the shmemory key*/
    return 0;
}

