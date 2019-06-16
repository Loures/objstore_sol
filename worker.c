#include <os_server.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <os_client.h>

void sync(void);	//the compiler complaints about implicit decleration otherwise

typedef struct buffer_t {
    char *data;
    size_t size;
} buffer_t;

static int iter_fd_exists(const void *ptr, void *arg) {
    int sfd = *(int*)arg;
    client_t *client = (client_t*)ptr;
    if (client->socketfd) return sfd == client->socketfd;
    return 0;
}

static size_t readn_polled(int fd, char *buff, size_t size) {
    size_t sizecnt = 0;

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){fd, POLLIN, 0};
    
    while(size > 0) {
        int ready = poll(pollfds, 1, 10);
        if (ready < 0) err_select(fd);

        if (ready == 1 && pollfds[0].revents & POLLIN) {
            ssize_t len = recv(fd, buff + sizecnt, size, 0);
            if (len <= 0) {
                if (len < 0) err_read(fd);
                return -1;
            }
            size = size - len;
            sizecnt = sizecnt + len;
        }
    }
    return sizecnt;
}

void worker_cleanup(int fd, client_t *client, char *buffer) {
    if (VERBOSE) {
        fprintf(stderr, "OBJSTORE: Cleaned up worker thread %ld\n", pthread_self());
    }
    free(buffer);
    if (client->name) free(client->name);
    linkedlist_iterative_remove(client_list, &iter_fd_exists, &fd);
    sync();
    close(fd);
    pthread_mutex_lock(&client_list_mtx);
    worker_num--;
    pthread_mutex_unlock(&client_list_mtx);
    if (worker_num <= 0) pthread_cond_signal(&worker_num_cond);
}

os_msg_t *worker_handlemsg(int fd, char *buff, size_t buffsize) {
    os_msg_t *msg = (os_msg_t*)malloc(sizeof(os_msg_t));
    memset(msg, 0, sizeof(os_msg_t));

    if (buffsize <= 0) return msg;

    char *saveptr;
    char *cmd = strtok_r(buff, " ", &saveptr);
    char *name = strtok_r(NULL, " ", &saveptr);
    char *len = strtok_r(NULL, " ", &saveptr);
    char *newline = strtok_r(NULL, " ", &saveptr);

    if (cmd) {
        msg->cmd = (char*)malloc(sizeof(char) * (strlen(cmd) + 1));     //there's always a cmd
        strcpy(msg->cmd, cmd);
    }

    if (name && name[0] != '\n') {
        msg->name = (char*)malloc(sizeof(char) * (strlen(name) + 1));     
        strcpy(msg->name, name);
    }
    
    if (len && len[0] != '\n') msg->len = atol(len);
    
    if (newline && newline[0] == '\n') {
        size_t headerlen = strlen(cmd) + 1 + strlen(name) + 1 + strlen(len) + 3;    //command name len \n data 
        msg->data = (char*)malloc(sizeof(char) * (msg->len));   
        memset(msg->data, 0, sizeof(char) * (msg->len));
        if (headerlen < buffsize) memcpy(msg->data, buff + headerlen, buffsize - headerlen);
        //handle more data
        if (msg->len > buffsize - headerlen) readn_polled(fd, msg->data + (buffsize - headerlen), msg->len - (buffsize - headerlen));
    }

    memset(buff, 0, SO_READ_BUFFSIZE);
    return msg;
}

void *worker_loop(void *ptr) {
    client_t *client = (client_t*)ptr;
    client->worker = pthread_self();

    if (VERBOSE) {
       fprintf(stderr, "OBJSTORE: Client connected on socket %d using thread %ld\n", client->socketfd, pthread_self());
    }
    
    int client_socketfd = client->socketfd;     //store socket fd on stack (faster access)

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){client_socketfd, POLLIN, 0};

    char *buffer = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buffer, 0, SO_READ_BUFFSIZE);

    setnonblocking(client_socketfd);

    while(OS_RUNNING && client->running == 1) {
        int ev = poll(pollfds, 1, 10);
        if (ev < 0) err_select(client_socketfd);

        if (ev == 1 && (pollfds[0].revents & POLLIN)) {
            size_t len = recv(client_socketfd, buffer, SO_READ_BUFFSIZE, 0);
            os_msg_t *msg = worker_handlemsg(client_socketfd, buffer, len);
            if (msg->cmd) {
                os_client_commandhandler(client_socketfd, client, msg);
            }
            free_os_msg(msg);
            if (len <= 0) {     //our client has shut itself down without disconnecting
                if (VERBOSE) {
                    fprintf(stderr, "OBJSTORE: Client on socket %d has terminated without disconnecting\n", client_socketfd);
                }
                break;
            }
        } 
    }
    worker_cleanup(client_socketfd, client, buffer);
    pthread_detach(pthread_self());
    return NULL;
}