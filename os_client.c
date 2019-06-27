#include <os_server.h>
#include <os_client.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fs.h>

const char *ok = "OK \n";

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
    
    linkedlist_elem *result = linkedlist_search(client_list, &iter_fd_exists, (void*)name);

    if (!result) {
        client->name = (char*)calloc(strlen(name) + 1, sizeof(char));     //+1 for \0 terminator
        if (client->name == NULL) {
            err_malloc(strlen(name) + 1);
            exit(EXIT_FAILURE);
        } 
        strcpy(client->name, name);

        fs_mkdir(client);

        if (VERBOSE) fprintf(stderr, "OBJSTORE: Client registered as %s\n", client->name);

        send_ok(fd);
    } else send_ko(fd, "Username already exists");
}

static void os_client_handleleave(int fd, client_t *client) {
    
    if (VERBOSE) {
        char *name = client->name;
        fprintf(stderr, "OBJSTORE: Client on socket %d (%s) left\n", fd, name);
    }

	send_ok(fd);
    client->running = 0;
}

static void os_client_handleretrieve(int fd, client_t *client, char *filename) {
    if (!fs_read(fd, client, filename)) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    }
}

static void os_client_handledelete(int fd, client_t *client, char *filename) {
    int err = fs_delete(client, filename);
    if (err == 0) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

static void os_client_handlestore(int fd, client_t *client, char *filename, char *data, size_t len, size_t datalen) {
    if (!client->name) {
        send_ko(fd, "You're not registered");
        return;
    }
    int err = fs_write(fd, client, filename, len, data, datalen);
    if (err == 0) {
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
            os_client_handlestore(fd, client, msg->name, msg->data, msg->len, msg->datalen);
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
            return 0;
            break;

        default:
            send_ko(fd, "Broken message");
            return 0;
            break;
    }
    return 0;
}
