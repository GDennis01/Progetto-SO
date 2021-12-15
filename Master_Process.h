struct msg_buf{
        long int type;
        int mtext[12];
    };
void read_macros(int fd,int * macros);