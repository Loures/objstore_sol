#include <os_server.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <ftw.h>
#include <worker.h>
#include <fs.h>

static ssize_t datadir_size = 0;
static ssize_t datadir_entries = 0;

void dispatcher_cleanup() {
    int err = close(os_serverfd);
    if (err < 0) err_close(os_serverfd);

    //Remove socket file at SOCKET_ADDR
    err = unlink(SOCKET_ADDR);
    if (err < 0) err_unlink(SOCKET_ADDR); 

    if (VERBOSE) fprintf(stderr, "OBJSTORE: %s succesfully unlinked\n", SOCKET_ADDR);

    //Free client_list from memory
    myhash_free(client_list, HASHTABLE_SIZE);
}

static int calc_stats(const char *fpath, const struct stat *sb, int typeflag) {
    if (typeflag == FTW_F) {
        datadir_size = datadir_size + sb->st_size;
        datadir_entries++;
    }
    return 0;
}

static void stats() {
	//Print amount of connected clients
    fprintf(stdout, "\nSTATS: Connected clients: %d\n", worker_num);

    //Do a file tree walk on the data folder and calculate size and number of files (i.e number of objects)
	datadir_size = 0;
	datadir_entries = 0;
    int err = ftw("data", &calc_stats, 64);
    if (err < 0) err_ftw("data");

    fprintf(stdout, "STATS: Data folder size: %ld bytes\n", datadir_size);
    fprintf(stdout, "STATS: Objects stored: %ld\n", datadir_entries);

}

void *dispatch(void *arg) {
    //Init pollfd struct
    struct pollfd pollfds[2];
    pollfds[0] = (struct pollfd){os_serverfd, POLLIN, 0};
    pollfds[1] = (struct pollfd){os_signalfd[0], POLLIN, 0};

    //Dumb buffer for signal pipe
    char empty[1];

    long maxclients = sysconf(_SC_OPEN_MAX);

    while(OS_RUNNING) {
        //Poll both the socket fd and the signal pipe fd
        int ev = poll(pollfds, 2, 10);    
        if (ev < 0) err_poll(os_serverfd);

        //Check if we have a pending connection, accept only if the number of connected clients is < 500 to avoid breaking the fd limit
        if (ev == 1 && (pollfds[0].revents & POLLIN) && worker_num < maxclients / 2 - 64) {   

            //Accept connection
            int client_fd = accept(os_serverfd, NULL, NULL);
            if (client_fd < 0) err_accept();

            int err = pthread_mutex_lock(&worker_num_mtx);
            if (err != 0) err_pthread("pthread_mutex_lock");


            worker_num++;   

            err = pthread_mutex_unlock(&worker_num_mtx);
            if (err != 0) err_pthread("pthread_mutex_unlock");


            //Create a worker thread for the client

            int *ptr = (int*)malloc(sizeof(int));
            if (ptr == NULL) {
                err_malloc((size_t)sizeof(int));
                exit(EXIT_FAILURE);
            }
        
            *ptr = client_fd;

            pthread_t wk;
            err = pthread_create(&wk, NULL, &worker_loop, (void*)ptr);
            if (err != 0) err_pthread("pthread_create");
            if (VERBOSE) fprintf(stderr, "OBJSTORE: Client connected on socket %d\n", client_fd);
        }
        

        //If there's something on the receiving end of the pipe (got SIGUSR1) then print stats
        if (ev == 1 && (pollfds[1].revents & POLLIN)) {
            read(os_signalfd[0], &empty, 1);
            stats();
        }
        
    }

    //Wait for all the worker threads to shutdown
    int err = pthread_mutex_lock(&worker_num_mtx);
    if (err != 0) err_pthread("pthread_mutex_lock");

    while (worker_num > 0) {
        err = pthread_cond_wait(&worker_num_cond, &worker_num_mtx);
        if (err != 0) err_pthread("pthread_cond_wait");
    }
    dispatcher_cleanup();

    err = pthread_mutex_unlock(&worker_num_mtx);
    if (err != 0) err_pthread("pthread_mutex_unlock");


    pthread_exit(NULL);
}
