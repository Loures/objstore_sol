#include <os_server.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <os_client.h>

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

void worker_cleanup(int fd, client_t *client) {
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Cleaned up worker thread %ld\n", pthread_self());
    //free(buffer);
    if (client->name) free(client->name);
    linkedlist_iterative_remove(client_list, &iter_fd_exists, &fd);
    close(fd);

    pthread_mutex_lock(&worker_num_mtx);
    worker_num--;
    if (worker_num <= 0) pthread_cond_signal(&worker_num_cond);
    pthread_mutex_unlock(&worker_num_mtx);

}

os_msg_t *worker_handlemsg(int fd, char *buff, size_t buffsize) {
    os_msg_t *msg = (os_msg_t*)calloc(1, sizeof(os_msg_t));
    if (msg == NULL) {
        err_malloc(sizeof(os_msg_t));
        exit(EXIT_FAILURE);
    } else {


    if (buffsize <= 0) return msg;


    char *saveptr;
    char *cmd = strtok_r(buff, " ", &saveptr);
    char *name = strtok_r(NULL, " ", &saveptr);
    char *len = strtok_r(NULL, " ", &saveptr);
    char *newline = strtok_r(NULL, " ", &saveptr);

    msg->datalen = 0;

    if (cmd) {
        msg->cmd = (char*)malloc(sizeof(char) * (strlen(cmd) + 1));     //there's always a cmd
        if (msg->cmd == NULL) {
            err_malloc(strlen(cmd) + 1);
            exit(EXIT_FAILURE);
        }
        
        strcpy(msg->cmd, cmd);
    }

    if (name && name[0] != '\n') {
        msg->name = (char*)malloc(sizeof(char) * (strlen(name) + 1)); 
        if (msg->name == NULL) {
            err_malloc(strlen(name) + 1);
            exit(EXIT_FAILURE);
        }
        
        strcpy(msg->name, name);
    }
    
    if (len && len[0] != '\n') msg->len = atol(len);
    
    if (newline && newline[0] == '\n') {
        size_t headerlen = strlen(cmd) + 1 + strlen(name) + 1 + strlen(len) + 3;    //command name len \n data 
        msg->data = (char*)calloc(msg->len, sizeof(char));
        if (msg->data == NULL) {
            err_malloc(msg->len);
            exit(EXIT_FAILURE);
        }
        
        if (headerlen < buffsize) {
            memcpy(msg->data, buff + headerlen, buffsize - headerlen);
            msg->datalen = buffsize - headerlen;    
        }
    }

    memset(buff, 0, SO_READ_BUFFSIZE);
    return msg;
    }    
}

void *worker_loop(void *ptr) {
    client_t *client = (client_t*)ptr;
    
    int client_socketfd = client->socketfd;     //store socket fd on stack (faster access)

    char buffer[SO_READ_BUFFSIZE];
    memset(buffer, 0, SO_READ_BUFFSIZE); 

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){client_socketfd, POLLIN, 0};

    while(OS_RUNNING && client->running == 1) {
        int ev = poll(pollfds, 1, 10);    //poll socket file descriptor
        if (ev < 0) err_select(client_socketfd);
        if (ev == 1 && (pollfds[0].revents & POLLIN)) {
            size_t len = recv(client_socketfd, (char*)buffer, SO_READ_BUFFSIZE, 0);
            os_msg_t *msg = worker_handlemsg(client_socketfd, (char*)buffer, len);
            if (msg->cmd) {
                os_client_commandhandler(client_socketfd, client, msg);
            }
            free_os_msg(msg);
            if (len <= 0) {     //our client has shut itself down without disconnecting
                if (VERBOSE) fprintf(stderr, "OBJSTORE: Client on socket %d has terminated without disconnecting\n", client_socketfd);
                break;
            }
        }
    } 
    worker_cleanup(client_socketfd, client);
    pthread_detach(pthread_self());
    return NULL;
}