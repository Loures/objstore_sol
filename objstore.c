#include <os_server.h>
#include <stdio.h>
#include <objstore.h>
#include <errormacros.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

typedef void (*sighandler_t)(int);

#define true 1
#define false 0

int objstore_fd = -1;
size_t SO_READ_BUFFSIZE = 0;
size_t LAST_LENGTH = 0;
sighandler_t prevsignal;
char objstore_errstr[256];  //reasonable length for a ko message

static size_t readn(int fd, char *buff, size_t size) {
    size_t sizecnt = 0;

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){fd, POLLIN, 0};
    while(size > 0) {
        int ready = poll(pollfds, 1, 10);
        if (ready < 0) err_select(fd);

        if (ready == 1 && pollfds[0].revents & POLLIN) {
            ssize_t len = recv(fd, buff + sizecnt, SO_READ_BUFFSIZE, 0);
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
    memset(objstore_errstr, 0, 256);
    strcpy(objstore_errstr, response);
    char first2[3];
    memset(first2, 0, 3);
    strncpy(first2, response, 2);
    if (strcmp(first2, "OK") == 0) {
        free(response);
        return true;   
    }
    free(response);
    return false;
}

int os_connect (char *name) {
    if (objstore_fd < 0) {
        struct sockaddr_un addr;
	    memset(&addr, 0, sizeof(struct sockaddr_un));
	    strncpy(addr.sun_path, SOCKET_ADDR, sizeof(addr.sun_path) - 1);
	    addr.sun_family = AF_UNIX;

        prevsignal = signal(SIGPIPE, SIG_IGN);
        if (prevsignal == SIG_ERR) {
            err_signal()
            return false;
        }

	    objstore_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	    int sockerr = connect(objstore_fd, (struct sockaddr*)&addr, sizeof(addr));
	    if (sockerr < 0) {
	    	err_socket(objstore_fd);
	    	return false;
	    }

	    ssize_t size = sizeof(size_t);
        getsockopt(objstore_fd, SOL_SOCKET, SO_RCVBUF, (void*)&SO_READ_BUFFSIZE, (socklen_t*)&size);
    }
    char *buff = (char*)calloc(SO_READ_BUFFSIZE, sizeof(char));

    dprintf(objstore_fd, "REGISTER %s \n", name);

    recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);
}

void *os_retrieve(char *name) {
    if (objstore_fd < 0) return NULL;
    dprintf(objstore_fd, "RETRIEVE %s \n", name);
    fflush(NULL);
    char *buff = (char*)calloc(SO_READ_BUFFSIZE, sizeof(char));
    char saveptr[SO_READ_BUFFSIZE];
    memset(saveptr, 0, SO_READ_BUFFSIZE);
    size_t buffsize = recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    
    char isok[5];
    memset(isok, 0, 5);
    strncpy(isok, buff, 4);
    if (strcmp(isok, "DATA") != 0) {
        check_response(buff);
        return NULL;
    } 

    char *storecmd = strtok_r(buff, " ", (char**)&saveptr);
    char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
    size_t len = atol(lenstr);
    
    char *data = (char*)calloc(len + 1, sizeof(char));     //null terminator!

    size_t headerlen = strlen(storecmd) + 1 + strlen(lenstr) + 3;   //1 is first space, 3 is " /n "
    if (headerlen < buffsize) memcpy(data, buff + headerlen, buffsize - headerlen);
    
    //handle more data
    if (len > buffsize - headerlen) readn(objstore_fd, data + (buffsize - headerlen), len - (buffsize - headerlen));
    
    LAST_LENGTH = len;
    
    free(buff);
    return (void*)data;
}

int os_store(char *name, void *block, size_t len) {
    if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "STORE %s %ld \n ", name, len);
    send(objstore_fd, block, len, 0);
    char *buff = (char*)calloc(SO_READ_BUFFSIZE, sizeof(char));
    recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);
}

int os_delete(char *name) {
    if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "DELETE %s \n", name);
    char *buff = (char*)calloc(SO_READ_BUFFSIZE, sizeof(char));
    recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    return check_response(buff);
}

int os_disconnect() {
    if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "LEAVE \n");
    close(objstore_fd);
    objstore_fd = -1;
    sighandler_t result = signal(SIGPIPE, prevsignal);
    if (result == SIG_ERR) {
        err_signal()
        return false;
    }
    return true;
}