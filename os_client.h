typedef struct os_client {
    int id;         //client id
    int socketfd[2]    //socket file descriptor - 0: read 1: write
    char* name;
} os_client

