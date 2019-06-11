#include <main.h>
#include <linkedlist.h>
#include <worker.h>
#include <errormacros.h>
#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>

pthread_t dispatcher_thread;
int serverfd;
struct sockaddr_un socket_address;

void dispatcher_cleanup() {
    int err = close(serverfd);
    if (err < 0) err_close(SOCKET_ADDR);
    err = unlink(SOCKET_ADDR);
    if (err < 0) err_unlink(SOCKET_ADDR); 
    #ifdef DEBUG
        fprintf(stderr, "DEBUG: %s succesfully unlinked\n", SOCKET_ADDR);
    #endif
}

void *dispatch(void *arg) {
    fd_set fdset;
    serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverfd < 0) err_socket();
    #ifdef DEBUG 
        fprintf(stderr, "DEBUG: Server socket created with fd %d\n", serverfd); 
    #endif
    memset(&socket_address, 0, sizeof(struct sockaddr_un));
    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, SOCKET_ADDR, sizeof(socket_address.sun_path) - 1); 
    int err = bind(serverfd, (const struct sockaddr*)&socket_address, sizeof(socket_address));
    if (err < 0) err_socket();
    #ifdef DEBUG 
        fprintf(stderr, "DEBUG: Server socket bound to %s\n", socket_address.sun_path); 
    #endif
    listen(serverfd, SOMAXCONN);
    FD_ZERO(&fdset);
    FD_SET(serverfd, &fdset);
    struct timespec timeout = (struct timespec){0, 10000000};
    while(OS_RUNNING) {
        int err = pselect(serverfd + 1, &fdset, NULL, NULL, &timeout, NULL);
        if (err < 0) err_select();
        printf("\t%d\n", err);
        if (FD_ISSET(serverfd, &fdset)) printf("INCOMING CONNECTION\n");
    }
    dispatcher_cleanup();
    pthread_exit(NULL);
}
