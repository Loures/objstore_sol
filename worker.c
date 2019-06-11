#include <main.h>

void *os_workerdummy(void *ptr) {
    client_t *client = (client_t*)ptr;
    printf("Client collegato sul socket %d", client->socketfd);
    pthread_detach(pthread_self());
    return NULL;
}