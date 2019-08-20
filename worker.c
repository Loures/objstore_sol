#include <os_server.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <os_client.h>

static int comp(const void *ptr, void *arg) {
    client_t *client = (client_t*)ptr;
    char *name = arg;
    if (strcmp(client->name, name) == 0) {
        free(client->name);
        client->name = NULL;
        free(client);
        client = NULL;
        return 1;
    }
    return 0;
}

void worker_cleanup(int fd, client_t *client) {
    if (VERBOSE) {
        if (client->name) fprintf(stderr, "OBJSTORE: Cleaned up client \'%s\' worker thread\n", client->name);
        else fprintf(stderr, "OBJSTORE: Cleaned up unregistered client (fd %d) worker thread\n", fd);
    }

    //Dealloc client struct, remove from client list and close fd, if client doesn't have a name, just free it.
    if (client->name) myhash_delete(client_list, HASHTABLE_SIZE, client->name, &comp, client->name);
    else {
        free(client);
        client = NULL;
    }
    close(fd);

    //We are done with this client, decrease number of worker threads
    pthread_mutex_lock(&worker_num_mtx);
    worker_num--;
    if (worker_num <= 0) pthread_cond_signal(&worker_num_cond);
    pthread_mutex_unlock(&worker_num_mtx);

}

os_msg_t *worker_handlemsg(int fd, char *buff, size_t buffsize) {
    //Init os_msg_t structure
    os_msg_t *msg = (os_msg_t*)calloc(1, sizeof(os_msg_t));
    if (msg == NULL) {
        err_malloc(sizeof(os_msg_t));
        exit(EXIT_FAILURE);
    } else {

    //If we haven't read nothing return an empty message
    if (buffsize <= 0) return msg;


    //Parse the header
    char *saveptr;
    char *cmd = strtok_r(buff, " ", &saveptr);
    char *name = strtok_r(NULL, " ", &saveptr);
    char *len = strtok_r(NULL, " ", &saveptr);
    char *newline = strtok_r(NULL, " ", &saveptr);

    //datalen contains how many bytes we read of the data to store in case of STORE command
    msg->datalen = 0;

    //Set command field of message
    if (cmd) {
        msg->cmd = (char*)malloc(sizeof(char) * (strlen(cmd) + 1));
        if (msg->cmd == NULL) {
            err_malloc(strlen(cmd) + 1);
            exit(EXIT_FAILURE);
        }
        
        strcpy(msg->cmd, cmd);
    }

    //Set name field of message if it exists
    if (name && name[0] != '\n') {
        msg->name = (char*)malloc(sizeof(char) * (strlen(name) + 1)); 
        if (msg->name == NULL) {
            err_malloc(strlen(name) + 1);
            exit(EXIT_FAILURE);
        }    
        strcpy(msg->name, name);
    }
    
    //Set len field of message if it exists
    if (len && len[0] != '\n') msg->len = atol(len);
    
    //Handle STORE data 
    if (newline && newline[0] == '\n') {
        //command name len \n data 
        size_t headerlen = strlen(cmd) + 1 + strlen(name) + 1 + strlen(len) + 3;
        
        //Alloc memory for data
        size_t calloc_size = 0;
        if (msg->len > SO_READ_BUFFSIZE) calloc_size = SO_READ_BUFFSIZE; else calloc_size = msg->len;
        msg->data = (char*)calloc(calloc_size, sizeof(char));
        if (msg->data == NULL) {
            err_malloc(SO_READ_BUFFSIZE);
            exit(EXIT_FAILURE);
        }
        
        //Alloc data field of message
        if (headerlen < buffsize) {
            memcpy(msg->data, buff + headerlen, buffsize - headerlen);
            msg->datalen = buffsize - headerlen;    
        }
    }

    //Reset buffer
    memset(buff, 0, SO_READ_BUFFSIZE);
    return msg;
    }    
}

static client_t *initclient(int fd) {
    client_t *client = (client_t*)calloc(1, sizeof(client_t));
    client->socketfd = fd;
    client->running = 1;
    client->name = NULL;
    client->worker = pthread_self();
    return client;
}

static int headercheck(char *buff, size_t len) {
    //5 is the length of the shortest command
    if (len < 5) return 0;

    //Handle STORE command
    if (strncmp(buff, "STORE", 5) == 0) {
        return strstr(buff, " \n ") != NULL;
    }

    //Handle everything else
    return strstr(buff, " \n") != NULL;
}

void *worker_loop(void *ptr) { 
     //store socket fd on stack (faster access)
    int client_socketfd = *(int*)ptr;    
    free(ptr);

    //Init read buffer on stack
    char buffer[SO_READ_BUFFSIZE];
    memset(buffer, 0, SO_READ_BUFFSIZE); 

    client_t *client = initclient(client_socketfd);

    size_t msg_len = 0;

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){client_socketfd, POLLIN, 0};

    while(OS_RUNNING && client->running) {
        //Start polling socket file descriptor
        int ev = poll(pollfds, 1, 10);    
        if (ev < 0) err_select(client_socketfd);
        if (ev == 1 && (pollfds[0].revents & POLLIN)) {
            size_t len = recv(client_socketfd, (char*)(buffer + msg_len), SO_READ_BUFFSIZE, 0);
            msg_len = msg_len + len;

            //Check if we got ATLEAST an header, otherwise keep reading (recv)
            if (headercheck(buffer, msg_len)) {
                os_msg_t *msg = worker_handlemsg(client_socketfd, (char*)buffer, msg_len);

                //Begin handling message
                if (msg->cmd) {
                    os_client_commandhandler(client_socketfd, client, msg);
                }

                //...then free it
                free_os_msg(msg);
                msg_len = 0;
            }

            //Client has terminated without disconnecting or something else bad happened
            if (len <= 0) {     
                if (VERBOSE) fprintf(stderr, "OBJSTORE: Client on fd %d disconnected\n", client_socketfd);
                break;
            }
        }
    }

    //Begin cleaning up worker thread
    worker_cleanup(client_socketfd, client);
    pthread_detach(pthread_self());
    return NULL;
}
