/*
	Ultime modifiche:
	-10/01/2022
		-updateInfos è stato spostato in node.h
	-30/12/2021
		-Aggiunta del prototipo del metodo "scritturaMastro"
*/
#include "common.c"
int scritturaMastro();
void updateInfos(int budget,int abort_trans,int my_index,int isAlive);
trans_pool createBlock(trans_pool p,int * sum_reward,transaction_block * block);