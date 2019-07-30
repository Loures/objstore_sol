#include <os_server.h>
#include <os_client.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fs.h>

const char *ok = "OK \n";

//Convert command string to int
static int getcommand(os_msg_t *msg) {
    if (msg->cmd) {
        if (strcmp(msg->cmd, "REGISTER") == 0 && msg->name) return OS_CLIENT_REGISTER;
        if (strcmp(msg->cmd, "STORE") == 0 && msg->name && msg->len && msg->data) return OS_CLIENT_STORE;
        if (strcmp(msg->cmd, "RETRIEVE") == 0 && msg->name) return OS_CLIENT_RETRIEVE;
        if (strcmp(msg->cmd, "DELETE") == 0 && msg->name) return OS_CLIENT_DELETE;
        if (strcmp(msg->cmd, "LEAVE") == 0) return OS_CLIENT_LEAVE;
    }

    //Not a command
    return -1;
}

//Helper functions for sending OKs or KOs
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

static client_t *os_client_handleregistration(int fd, client_t *client, char *name) {
    client->socketfd = fd;
    client->running = 1;

    client_t *dup = (client_t*)myhash_search(client_list, HASHTABLE_SIZE, name, &namecompare, name);
    if (dup) {
        send_ko(fd, "Username already registered");
        client->running = 0;
		return NULL;
    }

    client->name = (char*)calloc(strlen(name) + 1, sizeof(char));
    strcpy(client->name, name);

    
    myhash_insert(client_list, HASHTABLE_SIZE, client->name, client);
    fs_mkdir(client);


    send_ok(fd);

    if (VERBOSE) fprintf(stderr, "OBJSTORE: Client on fd %d registered as \'%s\'\n", fd, name);
    return client;
}

//LEAVE handler
static void os_client_handleleave(int fd, client_t *client) {
    
    if (VERBOSE) {
        char *name = client->name;
        fprintf(stderr, "OBJSTORE: Client on fd %d (%s) left\n", fd, name);
		
    }

	send_ok(fd);

    //Stop the client's worker thread
    client->running = 0;
}

//RETRIEVE handler
static void os_client_handleretrieve(int fd, client_t *client, char *filename) {
    if (!fs_read(fd, client, filename)) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    }
}

//DELETE handler
static void os_client_handledelete(int fd, client_t *client, char *filename) {
    int err = fs_delete(client, filename);
    if (err == 0) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

//STORE handler
static void os_client_handlestore(int fd, client_t *client, char *filename, char *data, size_t len, size_t datalen) {
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
                os_client_handleregistration(fd, client, msg->name);
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
                //What we got is not a message (???)
                send_ko(fd, "Broken message");
                return 0;
                break;
        }
    return 0;
}
