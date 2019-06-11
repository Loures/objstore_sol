#include <server.h>

void *workerdummy(void *ptr) {
    client_t *client = (client_t*)ptr;
    printf("Client collegato sul socket %d e operante sul thread %ld\n", client->socketfd, pthread_self());
    sleep(5);
    pthread_detach(pthread_self());
    return NULL;
}