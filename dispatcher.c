#include <os_server.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <ftw.h>
#include <worker.h>
#include <fs.h>

static ssize_t datadir_size = 0;
static ssize_t datadir_entries = 0;

static const char *ok = "OK \n";

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

static void send_ok(int fd) {
    send(fd, ok, 5, 0);
}

static void send_ko(int fd, const char *msg) {
    //5 is the length of "KO \n" and a null terminator
    char to_send[5 + strlen(msg)]; 
    sprintf(to_send, "KO %s\n", msg);
    send(fd, to_send, 5 + strlen(msg), 0);
}

static int namecompare(const void *ptr, void *arg) {
    client_t *client = (client_t*)ptr;
    char *name = (char*)arg;
    if (client->name) return strcmp(client->name, name) == 0;
    return 0;
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
    int err = ftw("data", &calc_stats, 64);
    if (err < 0) err_ftw("data");

    fprintf(stdout, "STATS: Data folder size: %ld bytes\n", datadir_size);
    fprintf(stdout, "STATS: Objects stored: %ld\n", datadir_entries);

}

client_t *handleregistration(int fd) {
    char buffer[SO_READ_BUFFSIZE];
    memset(buffer, 0, SO_READ_BUFFSIZE);
    recv(fd, (char*)buffer, SO_READ_BUFFSIZE, 0);

    char *saveptr;
    char *cmd = strtok_r(buffer, " ", &saveptr);
    char *name = strtok_r(NULL, " ", &saveptr);

    client_t *new = (client_t*)calloc(1, sizeof(client_t));
    
    new->socketfd = fd;
    new->running = 1;

    if (strcmp(cmd, "REGISTER") == 0 && strlen(name) > 0) {
        new->name = (char*)calloc(strlen(name) + 1, sizeof(char));
        strcpy(new->name, name);
        send_ok(fd);
    }

    client_t *client = (client_t*)myhash_search(client_list, HASHTABLE_SIZE, new->name, &namecompare, new->name);
    if (client) {
        close(fd);
        free(new->name);
        free(new);
        send_ko(fd, "Username already registered");
		return NULL;
    }

    return new;
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
        if (ev < 0) err_select(os_serverfd);

        //Check if we have a pending connection, accept only if the number of connected clients is < 500 to avoid breaking the fd limit
        if (ev == 1 && (pollfds[0].revents & POLLIN) && worker_num < maxclients / 2 - 64) {   

            //Accept connection
            int client_fd = accept(os_serverfd, NULL, NULL);
            if (client_fd < 0) {
                err_accept()
            }

            //Receive REGISTER and init client_t struct
            client_t *new = handleregistration(client_fd);
            if (new) {
                fs_mkdir(new);
                pthread_t *wk = &(new->worker);

                //Add the new client to the client list 
                myhash_insert(client_list, HASHTABLE_SIZE, new->name, new);

                pthread_mutex_lock(&worker_num_mtx);

                worker_num++;   

                pthread_mutex_unlock(&worker_num_mtx);

                //Create a worker thread for the client
                pthread_create(wk, NULL, &worker_loop, (void*)new);
                if (VERBOSE) fprintf(stderr, "OBJSTORE: Client connected on socket %d registered as \"%s\"\n", client_fd, new->name);
            }
        }

        //If there's something on the receiving end of the pipe (got SIGUSR1) then print stats
        if (ev == 1 && (pollfds[1].revents & POLLIN)) {
            read(os_signalfd[0], &empty, 1);
            stats();
        }
        
    }

    //Wait for all the worker threads to shutdown
    pthread_mutex_lock(&worker_num_mtx);

    while (worker_num > 0) pthread_cond_wait(&worker_num_cond, &worker_num_mtx);
    dispatcher_cleanup();

    pthread_mutex_unlock(&worker_num_mtx);
    
    pthread_exit(NULL);
}
