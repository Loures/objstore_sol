#include <os_server.h>
#include <linkedlist.h>
#include <worker.h>
#include <errormacros.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>

pthread_t dispatcher_thread;
int os_serverfd;

void dispatcher_cleanup() {
    int err = close(os_serverfd);
    if (err < 0) err_close(os_serverfd);
    err = unlink(SOCKET_ADDR);
    if (err < 0) err_unlink(SOCKET_ADDR); 
    #ifdef DEBUG
        fprintf(stderr, "DEBUG: %s succesfully unlinked\n", SOCKET_ADDR);
    #endif
    linkedlist_free(client_list);
}


void *dispatch(void *arg) {
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(os_serverfd, &fdset);
    struct timespec timeout = (struct timespec){0, 10000000};   //poll timeout (e.g poll every 20ms)
    while(OS_RUNNING) {
        int err = pselect(os_serverfd + 1, &fdset, NULL, NULL, &timeout, NULL);    //poll socket file descriptor
        if (err < 0) err_select(os_serverfd);
        if (FD_ISSET(os_serverfd, &fdset)) {   //checks if we have a pending connection and creates client shiet;
            int client_fd = accept(os_serverfd, NULL, NULL);

            
            client_t *new_client = (client_t*)malloc(sizeof(client_t));
            
            memset(new_client, 0, sizeof(client_t));    //zero client_t structure
            new_client->name = NULL;
            new_client->socketfd = client_fd;
            new_client->running = 1;
            pthread_t *wk = &(new_client->worker);
            //if (client_list) pthread_mutex_lock(&(client_list->prevmtx));
            //if (client_list) pthread_mutex_lock(&(client_list->nextmtx));
            client_list = linkedlist_new(client_list, (void*)new_client);
            worker_num++;   //add new thread worker
            //if (client_list) pthread_mutex_unlock(&(client_list->prevmtx));
            //if (client_list) pthread_mutex_unlock(&(client_list->nextmtx));
            pthread_create(wk, NULL, &worker_loop, (void*)new_client);
        }
        FD_ZERO(&fdset);
        FD_SET(os_serverfd, &fdset);
    }
    
    pthread_mutex_lock(&client_list_mtx);
    while (worker_num > 0) pthread_cond_wait(&worker_num_cond, &client_list_mtx);
    dispatcher_cleanup();
    pthread_mutex_unlock(&client_list_mtx);
    
    pthread_exit(NULL);
}
