#include <os_server.h>
#include <os_client.h>
#include <sys/socket.h>
#include <fs.h>

const char *ok = "OK \n";
const char *retrieve = "DATA ";

static int getcommand(os_msg_t *msg) {
    if (msg->cmd) {
        if (strcmp(msg->cmd, "REGISTER") == 0 && msg->name) return OS_CLIENT_REGISTER;
        if (strcmp(msg->cmd, "STORE") == 0 && msg->name && msg->len && msg->data) return OS_CLIENT_STORE;
        if (strcmp(msg->cmd, "RETRIEVE") == 0 && msg->name) return OS_CLIENT_RETRIEVE;
        if (strcmp(msg->cmd, "DELETE") == 0 && msg->name) return OS_CLIENT_DELETE;
        if (strcmp(msg->cmd, "LEAVE") == 0) return OS_CLIENT_LEAVE;
    }
    return -1;
}

static int iter_fd_exists(const void *ptr, void *arg) {
    int sfd = *(int*)arg;
    client_t *client = (client_t*)ptr;
    if (client->socketfd) return sfd == client->socketfd;
    return 0;
}

static void send_ok(int fd) {
    send(fd, ok, 5, 0);
}

static void send_ko(int fd, const char *msg) {
    char to_send[5 + strlen(msg)];  //5 is the length of "OK" plus "\n\0"
    sprintf(to_send, "KO %s\n", msg);
    send(fd, to_send, 5 + strlen(msg), 0);
}

static void os_client_handleregistration(int fd, client_t *client, const char *name) {
    if (client->name != NULL) {
        send_ko(fd, "You're already registered");
        return;
    }
        
    if (linkedlist_search(client_list, &iter_fd_exists, (void*)name) == NULL) {
        client->name = (char*)malloc(strlen(name) + 1);     //+1 for \0 terminator
        strcpy(client->name, name);

        fs_mkdir(client);

        #ifdef DEBUG
            fprintf(stderr, "DEBUG: Client registered as %s\n", client->name);
        #endif

        send_ok(fd);
    } else send_ko(fd, "Username already exists");
}

static void os_client_handleleave(int fd, client_t *client) {
    
    #ifdef DEBUG
        char *name = client->name;
        fprintf(stderr, "DEBUG: Client on socket %d (%s) left\n", fd, name);
    #endif

    client->running = 0;
}

static void os_client_handleretrieve(int fd, client_t *client, char *filename) {
    if (!client->name) {
        send_ko(fd, "You're not registered");
        return;
    }

    os_read_t read_t = fs_read(client, filename);
    if (read_t.data) {
        char len[sizeof(ssize_t) + 1];
        memset(len, 0, sizeof(ssize_t) + 1);
        sprintf(len, "%ld", read_t.size);
        ssize_t response_len = strlen(retrieve) + strlen(len) + 3 + read_t.size;
        char *response = (char*)malloc(sizeof(char) * (response_len));   //3 -> space newline space
        memset(response, 0, response_len);
        sprintf(response, "DATA %s \n ", len);
        memcpy(response + (strlen(retrieve) + strlen(len) + 3), read_t.data, read_t.size);
        send(fd, response, response_len, 0);
        free(response);
        free(read_t.data);
    } else {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    }
}

static void os_client_handledelete(int fd, client_t *client, char *filename) {
    if (!client->name) {
        send_ko(fd, "You're not registered");
        return;
    }

    int err = fs_delete(client, filename);
    if (err == -1) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

static void os_client_handlestore(int fd, client_t *client, char *filename, char *data, size_t len) {
    if (!client->name) {
        send_ko(fd, "You're not registered");
        return;
    }
    int err = fs_write(client, filename, len, data);
    if (err == -1) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

int os_client_commandhandler(int fd, client_t *client, os_msg_t *msg) {
    int cmd_num = getcommand(msg);
    switch (cmd_num) {

        case OS_CLIENT_REGISTER: 
            os_client_handleregistration(fd, client, (const char*)msg->name);
            return 0;
            break;
        case OS_CLIENT_STORE: 
            os_client_handlestore(fd, client, msg->name, msg->data, msg->len);
            return 0;
            break;
        case OS_CLIENT_RETRIEVE: 
            os_client_handleretrieve(fd, client, msg->name);
            break;
        case OS_CLIENT_DELETE:
            os_client_handledelete(fd, client, msg->name);
            break;
        case OS_CLIENT_LEAVE: 
            os_client_handleleave(fd, client);
            return -1;
            /* code */
            break;

        default:
            send_ko(fd, "Broken message");
            return 0;
            break;
    }
    return 0;
}