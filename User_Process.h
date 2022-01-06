/*
	Ultime modifiche:
	-06/01/2022
		-Aggiunta della lista "trans_sent" e del suo relativo indice trans_sent_index
		-Aggiunta della macro MAX_LOCAL_TRANS

	-30/12/2021
		-Rimozione del prototipo del metodo "creaTransazione" -> spostato in "common.h"
*/
#define _GNU_SOURCE  /* Per poter compilare con -std=c89 -pedantic */
#define MAX_LOCAL_TRANS 100000 /*Maximum transactions saved locally by each user*/
#include "common.c"


transaction* trans_sent;/*list containing all transaction sent but not yet written in the ledger*/
int trans_sent_Index=0; /*max index of the transaction list*/

void printTransaction(struct transaction tr);
int getRndNode();
int getBalance();
int checkLedger(transaction tr);
transaction* removeTrans(transaction tr);