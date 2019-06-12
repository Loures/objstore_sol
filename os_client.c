#include <os_server.h>
#include <os_client.h>
#include <sys/socket.h>

static int getcommand(const char* command) {
    if (!strcmp(command, "REGISTER")) return OS_CLIENT_REGISTER;
    if (!strcmp(command, "STORE")) return OS_CLIENT_STORE;
    if (!strcmp(command, "RETRIEVE")) return OS_CLIENT_RETRIEVE;
    if (!strcmp(command, "DELETE")) return OS_CLIENT_DELETE;
    if (!strcmp(command, "LEAVE")) return OS_CLIENT_LEAVE;
    return -1;
}

char *inttochar(int from, char *to) {
	int temp = from;
    memcpy(to, &temp, sizeof(int));
    return to;
}

static int iter_checkdup(const void *ptr, void *arg) {
    char *name = (char*)arg;
    client_t *client = (client_t*)ptr;
    if (client->name) return strcmp(client->name, name) == 0;
    return 0;
}


static void client_send_headerlen(int fd, int len) {
    char clen[sizeof(int)];
    inttochar(len, clen);
    send(fd, clen, sizeof(int), 0);
}

void client_send_reply(int fd, char *receipt) {
    client_send_headerlen(fd, strlen(receipt) + 1);     //+1 for null terminator
    send(fd, receipt, strlen(receipt) + 1, 0);
}

static void send_ok(int fd, const char *msg) {
    char to_send[5 + strlen(msg)];  //5 is the length of "OK" plus "\n\0"
    sprintf(to_send, "OK %s\n", msg);
    client_send_reply(fd, to_send);
}

static void send_ko(int fd, const char *msg) {
    char to_send[5 + strlen(msg)];  //5 is the length of "OK" plus "\n\0"
    sprintf(to_send, "KO %s\n", msg);
    client_send_reply(fd, to_send);
}

static void os_client_handleregistration(int fd, client_t *client, const char *name) {
    pthread_mutex_lock(&client_list_mtx);
    if (linkedlist_search(client_list, &iter_checkdup, (void*)name) == NULL) {
        client->name = (char*)malloc(strlen(name) + 1);     //+1 for \0 terminator
        strcpy(client->name, name);

        #ifdef DEBUG
            printf("DEBUG: Client registered as %s\n", client->name);
        #endif

        send_ok(fd, "Registered");
    } else send_ko(fd, "Username already exists");
    pthread_mutex_unlock(&client_list_mtx);
}

static void os_client_handleleave(int fd, client_t *client) {
    pthread_mutex_lock(&client_list_mtx);
    char *name = client->name;
    linkedlist_elem *torm;
    if ((torm = linkedlist_search(client_list, &iter_checkdup, (void*)name)) != NULL) {

        #ifdef DEBUG
            printf("DEBUG: Client %s left\n", name);
        #endif

        free(client->name);
        client_list = linkedlist_remove(client_list, torm);
        
        send_ok(fd, "Goodbye!");
        
        close(fd);
    } else send_ko(fd, "You're not registered");
    pthread_mutex_unlock(&client_list_mtx);
}

int os_client_commandhandler(client_t *client, int fd, char *header, size_t headerlen) {
    char cpy[headerlen];
    char saveptr[headerlen];   //for strtok_r;
    strcpy(cpy, (const char*)header);
    char *command = strtok_r(cpy, " ", (char**)&saveptr);
    int cmd_num;

    char *name;
    char *len;

    if (command) cmd_num = getcommand(command); else cmd_num = getcommand(cpy);
    switch (cmd_num) {

        case OS_CLIENT_REGISTER: 
            name = strtok_r(NULL, " ", (char**)&saveptr);
            os_client_handleregistration(fd, client, (const char*)name);
            return 0;
            break;
        case OS_CLIENT_STORE: 
            printf("GOT STORE\n");
            /* code */
            break;
        case OS_CLIENT_RETRIEVE: 
            printf("GOT RETRIEVE\n");
            /* code */
            break;
        case OS_CLIENT_DELETE: 
            printf("GOT DELETE\n");
            /* code */
            break;
        case OS_CLIENT_LEAVE: 
            os_client_handleleave(fd, client);
            return 0;
            /* code */
            break;

        default:
            return 0;
            break;
    }
    return 0;
}