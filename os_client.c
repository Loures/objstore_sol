#include <os_server.h>
#include <os_client.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <fs.h>

const char *ok = "OK \n";

static int create_lockfile(const char *name) {
    char buff[512];
    memset(buff, 0, 512);
    sprintf(buff, "data/%s/.lock", name);
    int fd = open(buff, O_CREAT | O_EXCL);
    if (fd < 0 && errno != EEXIST) {
        err_open(buff);
        return 0;
    }
    if (fd > 0) {
        int err = close(fd);
        if (err < 0) err_close(fd);
    }
    return fd > 0;
}

int unlink_lockfile(const char *name) {
    char buff[512];
    memset(buff, 0, 512);
    sprintf(buff, "data/%s/.lock", name);
    int err = unlink(buff);
    if (err < 0) {
        err_unlink(buff);
        return 0;
    }
    return err >= 0;
}

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
    ssize_t result = sendn(fd, ok, 5, 0);

    if (result < 0) err_write(fd);

}

static void send_ko(int fd, const char *msg) {
    //5 is the length of "KO \n" and a null terminator
    char to_send[5 + strlen(msg)]; 
    sprintf(to_send, "KO %s\n", msg);
    ssize_t result = sendn(fd, to_send, 5 + strlen(msg), 0);
    
    if (result < 0) err_write(fd);

}

//static int namecompare(const void *ptr, void *arg) {
//    client_t *client = (client_t*)ptr;
//    char *name = (char*)arg;
//    if (client->name) return strcmp(client->name, name) == 0;
//    return 0;
//}

static int os_client_handleregistration(int fd, char *name, char *clientname) {
    //client->socketfd = fd;
    //client->running = 1;
    fs_mkdir(clientname);

    //client_t *dup = (client_t*)myhash_search(client_list, HASHTABLE_SIZE, name, &namecompare, name);
    if (!create_lockfile(clientname)) {
        send_ko(fd, "Username already registered");
        //client->running = 0;
		return 1;
    }

    strncpy(name, clientname, 255);

    //client->name = (char*)calloc(strlen(name) + 1, sizeof(char));
    //strcpy(client->name, name);

    
    //myhash_insert(client_list, HASHTABLE_SIZE, client->name, client);


    send_ok(fd);

    if (VERBOSE) fprintf(stderr, "OBJSTORE: Client on fd %d registered as \'%s\'\n", fd, name);
    return 0;
}

//LEAVE handler
static int os_client_handleleave(int fd, const char *name) {
    
    if (VERBOSE) {
        //char *name = client->name;
        fprintf(stderr, "OBJSTORE: Client on fd %d (%s) left\n", fd, name);
		
    }

	send_ok(fd);

    //Stop the client's worker thread
    return 1;
}

//RETRIEVE handler
static void os_client_handleretrieve(int fd, const char *name, char *filename) {
    if (!fs_read(fd, name, filename)) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    }
}

//DELETE handler
static void os_client_handledelete(int fd, const char *name, char *filename) {
    int err = fs_delete(name, filename);
    if (err == 0) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

//STORE handler
static void os_client_handlestore(int fd, const char *name, char *filename, char *data, size_t len, size_t datalen) {
    int err = fs_write(fd, name, filename, len, data, datalen);
    if (err == 0) {
        char buf[128];
        strerror_r(errno, buf, 128);
        send_ko(fd, buf);
    } else send_ok(fd);
}

int os_client_commandhandler(int fd, char *name, os_msg_t *msg) {
    int cmd_num = getcommand(msg);
        switch (cmd_num) {
            case OS_CLIENT_REGISTER: 
                return os_client_handleregistration(fd, name, msg->name);
                break;
            case OS_CLIENT_STORE: 
                os_client_handlestore(fd, name, msg->name, msg->data, msg->len, msg->datalen);
                return 0;
                break;
            case OS_CLIENT_RETRIEVE: 
                os_client_handleretrieve(fd, name, msg->name);
                break;
            case OS_CLIENT_DELETE:
                os_client_handledelete(fd, name, msg->name);
                break;
            case OS_CLIENT_LEAVE: 
                return os_client_handleleave(fd, name);
                break;

            default:
                //What we got is not a message (???)
                send_ko(fd, "Broken message");
                return 0;
                break;
        }
    return 0;
}
