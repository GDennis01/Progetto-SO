/*
	Ultime modifiche:
	-30/12/2021
		-Rimozione del prototipo del metodo "creaTransazione" -> spostato in "common.h"
*/
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
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
#include "common.c"
void printTransaction(struct transaction tr);
int getRndNode();