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
    if (VERBOSE) fprintf(stderr, "OBJSTORE: %s succesfully unlinked\n", SOCKET_ADDR);
    linkedlist_free(client_list);
}


static void stats() {
	fflush(stdout);
	fprintf(stdout, "\nSTATS: Connected clients: %d\n", worker_num);
	FILE *size = popen("du -cbs --apparent-size data/*/* 2> /dev/null | tail -1 | cut -f1 ", "r");
	char buff[128];
	memset(buff, 0, 128);
	fgets(buff, 128, size);
	fprintf(stdout, "STATS: Store total size: %ld bytes\n", atol(buff));
	pclose(size);
	FILE *count = popen("ls data/*/* 2> /dev/null | wc -w", "r");
	memset(buff, 0, 128);
	fgets(buff, 128, count);
	fprintf(stdout, "STATS: Objects stored: %ld\n\n", atol(buff));
	pclose(count);
	fflush(stdout);
}


void *dispatch(void *arg) {
    struct pollfd pollfds[2];
    pollfds[0] = (struct pollfd){os_serverfd, POLLIN, 0};
    pollfds[1] = (struct pollfd){os_signalfd[0], POLLIN, 0};

    char empty[1];

    while(OS_RUNNING) {
        int ev = poll(pollfds, 2, 10);    //poll socket file descriptor
        if (ev < 0) err_select(os_serverfd);

        if (ev == 1 && (pollfds[0].revents & POLLIN)) {   //checks if we have a pending connection and creates client shiet;
            int client_fd = accept(os_serverfd, NULL, NULL);

            client_t *new_client = (client_t*)calloc(1, sizeof(client_t));
            if (new_client == NULL) {
                err_malloc(sizeof(client_t));
                exit(EXIT_FAILURE);
            }    
           
		   	new_client->name = NULL;
            new_client->socketfd = client_fd;
            new_client->running = 1;

            pthread_t *wk = &(new_client->worker);

            linkedlist_elem *newelem = linkedlist_new(client_list, (void*)new_client);

            pthread_mutex_lock(&worker_num_mtx);
            
            client_list = newelem; 
            worker_num++;   //add new thread worker

            pthread_mutex_unlock(&worker_num_mtx);

            pthread_create(wk, NULL, &worker_loop, (void*)new_client);
            if (VERBOSE) fprintf(stderr, "OBJSTORE: Client connected on socket %d\n", client_fd);
           
        }

        if (ev == 1 && (pollfds[1].revents & POLLIN)) {
            read(os_signalfd[0], &empty, 1);
            stats();
        }
        
    }
    pthread_mutex_lock(&worker_num_mtx);
    while (worker_num > 0) {
        pthread_cond_wait(&worker_num_cond, &worker_num_mtx);
    }
    dispatcher_cleanup();
    pthread_mutex_unlock(&worker_num_mtx);
    
    pthread_exit(NULL);
}
