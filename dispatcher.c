#include <server.h>
#include <linkedlist.h>
#include <worker.h>
#include <errormacros.h>
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>

pthread_t dispatcher_thread;
int serverfd;

void dispatcher_cleanup() {
    int err = close(serverfd);
    if (err < 0) err_close(serverfd);
    err = unlink(SOCKET_ADDR);
    if (err < 0) err_unlink(SOCKET_ADDR); 
    #ifdef DEBUG
        fprintf(stderr, "DEBUG: %s succesfully unlinked\n", SOCKET_ADDR);
    #endif
}


void *dispatch(void *arg) {
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(serverfd, &fdset);
    struct timespec timeout = (struct timespec){0, 20000000};   //poll timeout (e.g poll every 20ms)
    while(OS_RUNNING) {
        int err = pselect(serverfd + 1, &fdset, NULL, NULL, &timeout, NULL);    //poll socket file descriptor
        if (err < 0) err_select();
        if (FD_ISSET(serverfd, &fdset)) {   //checks if we have a pending connection and creates client shiet;
            int client_fd = accept(serverfd, NULL, NULL);
            client_t *new_client = (client_t*)malloc(sizeof(client_t));
            memset(new_client, 0, sizeof(client_t));
            new_client->name = NULL;
            new_client->socketfd = client_fd;
            client_list = linkedlist_new(client_list, (void*)new_client);
            pthread_create(&(new_client->worker), NULL, &workerdummy, (void*)new_client);
        }
        FD_ZERO(&fdset);
        FD_SET(serverfd, &fdset);
    }
    dispatcher_cleanup();
    pthread_exit(NULL);
}
