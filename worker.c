#include <os_server.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <os_client.h>
#include <errormacros.h>

void worker_cleanup() {
    worker_num--;
    if (worker_num == 0) pthread_cond_signal(&worker_num_cond);
}

ssize_t worker_recv_headerlen(int fd) {     //read header length
    char recvnum[sizeof(int)];
    memset(recvnum, 0, sizeof(int));
    int r = recv(fd, recvnum, sizeof(int), 0);
    return (ssize_t)(*(int*)recvnum);
}

char *worker_recv_header(int fd) {    //read header
    ssize_t size = worker_recv_headerlen(fd);
    char *buf = (char*)malloc(size + 1); 
    recv(fd, buf, size, 0);
    buf[size - 1] = '\0';
    return buf;
}

void *worker_loop(void *ptr) {
    client_t *client = (client_t*)ptr;
    client->worker = pthread_self();

    #ifdef DEBUG
       printf("DEBUG: Client connected on socket %d using thread %ld\n", client->socketfd, pthread_self());
    #endif
    
    int client_socketfd = client->socketfd;     //store socket fd on stack (faster access)

    fd_set r_fdset;
    FD_ZERO(&r_fdset);
    FD_SET(client_socketfd, &r_fdset);
    struct timespec timeout = (struct timespec){0, 100000000};
    while(OS_RUNNING && fcntl(client_socketfd, F_GETFD) > -1) {
        int chbits = pselect(client_socketfd + 1, &r_fdset, NULL, NULL, &timeout, NULL);
        if (chbits < 0) err_select(client_socketfd);
        if (FD_ISSET(client_socketfd, &r_fdset)) {
            char *header = worker_recv_header(client_socketfd);
            os_client_commandhandler(client, client_socketfd, header, strlen(header) + 1);  //+1 for null terminator
            free(header);
        }
        FD_ZERO(&r_fdset);
        FD_SET(client_socketfd, &r_fdset);
    }
    worker_cleanup();
    pthread_detach(pthread_self());
    return NULL;
}