struct msg_buf{
        long int type;
        int *mtext;
    };
void read_macros(int fd,int * macros);