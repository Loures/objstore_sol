#include <os_server.h>
#include <sys/select.h>
#include <errormacros.h>

int iter(const void *ptr, void *arg) {
    return ptr == arg;
}

void worker_cleanup(client_t *client) {
    printf("cleanup thread %ld\n", client->worker);
    worker_num--;
    if (worker_num == 0) pthread_cond_signal(&worker_num_cond);
}

void *worker_loop(void *ptr) {
    client_t *client = (client_t*)ptr;
    client->worker = pthread_self();

    #ifdef DEBUG
       printf("Client connected on socket %d using thread %ld\n", client->socketfd, pthread_self());
    #endif
    
    int client_socketfd = client->socketfd;     //store socket fd on stack (faster access)

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(client_socketfd, &fdset);
    struct timespec timeout = (struct timespec){0, 10000000};
    while(OS_RUNNING) {
        int chbits = pselect(client_socketfd + 1, &fdset, NULL, NULL, &timeout, NULL);
        if (chbits < 0) err_select(client_socketfd);
    }
    worker_cleanup(client);
    pthread_detach(pthread_self());
    return NULL;
}