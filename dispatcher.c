#include <os_server.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <worker.h>

#define SOCKET_ADDR "/tmp/objstore.sock"


pthread_t dispatcher_thread;
int os_serverfd;

void dispatcher_cleanup() {
    int err = close(os_serverfd);
    if (err < 0) err_close(os_serverfd);
    err = unlink(SOCKET_ADDR);
    if (err < 0) err_unlink(SOCKET_ADDR); 
    if (VERBOSE) {
        fprintf(stderr, "OBJSTORE: %s succesfully unlinked\n", SOCKET_ADDR);
    }
    linkedlist_free(client_list);
}


void *dispatch(void *arg) {
    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){os_serverfd, POLLIN, 0};

    while(OS_RUNNING) {
        int ev = poll(pollfds, 1, 10);    //poll socket file descriptor
        if (ev < 0) err_select(os_serverfd);
        if (ev == 1 && (pollfds[0].revents & POLLIN)) {   //checks if we have a pending connection and creates client shiet;
            int client_fd = accept(os_serverfd, NULL, NULL);

            client_t *new_client = (client_t*)malloc(sizeof(client_t));
            memset(new_client, 0, sizeof(client_t));    //zero client_t structure

            new_client->name = NULL;
            new_client->socketfd = client_fd;
            new_client->running = 1;

            pthread_t *wk = &(new_client->worker);

            pthread_mutex_lock(&client_list_mtx);

            client_list = linkedlist_new(client_list, (void*)new_client);
            worker_num++;   //add new thread worker

            pthread_mutex_unlock(&client_list_mtx);

            pthread_create(wk, NULL, &worker_loop, (void*)new_client);
        }
    }
    pthread_mutex_lock(&client_list_mtx);
    while (worker_num > 0) {
        pthread_cond_wait(&worker_num_cond, &client_list_mtx);
    }
    dispatcher_cleanup();
    pthread_mutex_unlock(&client_list_mtx);
    
    pthread_exit(NULL);
}
