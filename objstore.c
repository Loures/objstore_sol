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
#define ERRSTR_LEN 256

int objstore_fd = -1;
size_t SO_READ_BUFFSIZE = 1024;     //reasonable default value
size_t LAST_LENGTH = 0;
sighandler_t prevsignal;
char objstore_errstr[ERRSTR_LEN];  //reasonable length for a ko message

static size_t readn(int fd, char *buff, size_t size) {
    size_t sizecnt = 0;

    struct pollfd pollfds[1];
    pollfds[0] = (struct pollfd){fd, POLLIN, 0};
    while(sizecnt < size) {
        int ready = poll(pollfds, 1, 10);
        if (ready < 0) err_select(fd);

        if (ready == 1 && pollfds[0].revents & POLLIN) {
            size_t len = recv(fd, buff + sizecnt, SO_READ_BUFFSIZE, 0);
            if (len <= 0) {
                if (len < 0) err_read(fd);
                return false;
            }
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
        //free(response);
        return true;   
    }
    //free(response);
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
    char buff[ERRSTR_LEN];
    memset(buff, 0, ERRSTR_LEN);

    dprintf(objstore_fd, "REGISTER %s \n", name);

    recv(objstore_fd, buff, ERRSTR_LEN, 0);
    return check_response((char*)buff);
}

void *os_retrieve(char *name) {
    if (objstore_fd < 0) return NULL;
    dprintf(objstore_fd, "RETRIEVE %s \n", name);
    fflush(NULL);

    char buff[SO_READ_BUFFSIZE];
    char saveptr[SO_READ_BUFFSIZE];
    
    memset(saveptr, 0, SO_READ_BUFFSIZE);
    memset(buff, 0, SO_READ_BUFFSIZE);

    size_t buffsize = recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    
    char isok[5];   //length of "OK \n" is 4 + null terminator
    memset(isok, 0, 5);

    strncpy(isok, buff, 4);
    if (strcmp(isok, "DATA") != 0) {
        check_response(buff);
        return NULL;
    } 

    char *storecmd = strtok_r(buff, " ", (char**)&saveptr);
    char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
    size_t len = atol(lenstr);
    
    printf("\tlen: %ld\n", len);

    char *data = (char*)calloc(len + 1, sizeof(char));     //null terminator!

    size_t headerlen = strlen(storecmd) + 1 + strlen(lenstr) + 3;   //1 is first space, 3 is " /n "
    if (headerlen < buffsize) memcpy(data, buff + headerlen, buffsize - headerlen);
    
    //handle more data
    size_t readn_len = true;
    if (len > buffsize - headerlen) readn_len = readn(objstore_fd, data + (buffsize - headerlen), len - (buffsize - headerlen));
    
    LAST_LENGTH = len;
    
    if (readn_len) return (void*)data;
    else return NULL;
}

int os_store(char *name, void *block, size_t len) {
    if (objstore_fd < 0) return false;
    char header[128];
    memset(header, 0, 128);
    sprintf((char*)header, "STORE %s %ld \n ", name, len);

    ssize_t headerlen = strlen(header);

    char *tosend = (char*)calloc(headerlen + len, sizeof(char));
    memcpy(tosend, header, headerlen);
    memcpy(tosend + headerlen, block, len);
    send(objstore_fd, tosend, headerlen + len, 0);

    free(tosend);
    char buff[ERRSTR_LEN];
    memset(buff, 0, ERRSTR_LEN);

    size_t recv_len = recv(objstore_fd, buff, ERRSTR_LEN, 0);
    if (recv_len <= 0) {
        if (recv_len < 0) err_read(objstore_fd);
        return false;
    }   
    return check_response(buff);
}

int os_delete(char *name) {
    if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "DELETE %s \n", name);

    char buff[ERRSTR_LEN];
    memset(buff, 0, ERRSTR_LEN);
    
    recv(objstore_fd, buff, ERRSTR_LEN, 0);
    return check_response(buff);
}

int os_disconnect() {
    if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "LEAVE \n");

    char buff[5];         //length of "OK \n" is 4 + null terminator
    memset(buff, 0, 5);   
    recv(objstore_fd, buff, 5, 0);

    close(objstore_fd);
    objstore_fd = -1;
    sighandler_t result = signal(SIGPIPE, prevsignal);
    if (result == SIG_ERR) {
        err_signal()
        return false;
    }
    return check_response(buff);
}