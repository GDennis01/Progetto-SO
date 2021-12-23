
#include "common.h"


void read_macros(int fd,int * macros){
    int j=0,i,tmp;
    char c;
    char *string=malloc(10);/*Macros' parameter cant have more than 10 digits*/
    while(read(fd,&c,1) != 0){
        if(c == ':'){ 
            read(fd,&c,1);
            for( i=0;c>=48 && c<=57;i++){
                *(string+i)=c;
                read(fd,&c,1);
            }
            tmp = atoi(string);
            printf("%d°:%d\n",j+1,tmp);
            bzero(string,10);/*Erasing the temporary string since it may cause data inconsistency*/
            macros[j]=tmp;
            j++;
        }
    }
    for(i=0;i<sizeof(macros);i++){
        printf("MACRO:%d\n",macros[i]);
    }
}