#include <os_server.h>
#include <stdio.h>
#include <objstore.h>
#include <errormacros.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

int socketfd = -1;
size_t SO_READ_BUFFSIZE = 0;
size_t LAST_LENGTH = 0;

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

static int check_response(char *response) {
    char first2[2];
    strncpy(first2, response, 2);
    printf(response);
    if (strcmp(first2, "OK") == 0) {
        //fprintf(stderr, "OK \n");
        //printf("what?");
        //free(response);
        return 1;
    }
    //printf(response);
    //free(response);
    return -1;
}

static char* getreply() {
    char *buff = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);
    recv(socketfd, buff, SO_READ_BUFFSIZE, 0);
    return buff;
}

int os_connect (char *name) {
    if (socketfd < 0) {
        struct sockaddr_un addr;
	    memset(&addr, 0, sizeof(struct sockaddr_un));
	    strncpy(addr.sun_path, SOCKET_ADDR, sizeof(addr.sun_path) - 1);
	    addr.sun_family = AF_UNIX;

	    socketfd = socket(AF_UNIX, SOCK_STREAM, 0);
	    int sockerr = connect(socketfd, (struct sockaddr*)&addr, sizeof(addr));
	    if (sockerr < 0) {
	    	err_socket(socketfd);
	    	return -1;
	    }

	    ssize_t size = sizeof(size_t);
        getsockopt(socketfd, SOL_SOCKET, SO_RCVBUF, (void*)&SO_READ_BUFFSIZE, (socklen_t*)&size);
    }

    dprintf(socketfd, "REGISTER %s \n", name);

    char *response = getreply();
    return check_response(response);
}

void *os_retrieve(char *name) {
    dprintf(socketfd, "RETRIEVE %s \n", name);
    fflush(NULL);
    char *buff = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    char *saveptr = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);
    memset(saveptr, 0, SO_READ_BUFFSIZE);
    size_t buffsize = recv(socketfd, buff, SO_READ_BUFFSIZE, 0);
    char isok[4];
    strncpy(isok, buff, 4);
    if (strcmp(isok, "DATA") != 0) {
        check_response(buff);
        return NULL;
    } 

    char *storecmd = strtok_r(buff, " ", (char**)&saveptr);
    char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
    size_t len = atol(lenstr);
    char *data = (char*)malloc(sizeof(char) * len);
    memset(data, 0, len);

    size_t headerlen = strlen(storecmd) + 1 + strlen(lenstr) + 3;   //1 is first space, 3 is " /n "
    if (headerlen < buffsize) memcpy(data, buff + headerlen, buffsize - headerlen);
    //handle more data
    if (len > buffsize - headerlen) readn_polled(socketfd, data + (buffsize - headerlen), len - (buffsize - headerlen));
    LAST_LENGTH = len;
    return (void*)data;
}

int os_store(char *name, void *block, size_t len) {
    dprintf(socketfd, "STORE %s %ld \n ", name, len);
    send(socketfd, block, len, 0);
    char *buff = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);
    recv(socketfd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);
}

int os_delete(char *name) {
    dprintf(socketfd, "DELETE %s \n", name);
    char *buff = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);
    recv(socketfd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);
}

int os_disconnect(char *name) {
    dprintf(socketfd, "LEAVE \n");
    char *buff = (char*)malloc(sizeof(char) * SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);
    recv(socketfd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);  
}