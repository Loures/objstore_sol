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

//Reasonable default value
size_t SO_READ_BUFFSIZE = 1024;

//Contains the length of the last os_retrieve read
size_t LAST_LENGTH = 0;

sighandler_t prevsignal;

//Contains last error message (KO ...)
char objstore_errstr[ERRSTR_LEN];

static int check_response(char *response) {
    //Reset objstore_errstr and copy command response into it
    memset(objstore_errstr, 0, 256);
    strcpy(objstore_errstr, response);

    //Grab first 2 chars of response
    char first2chars[3];
    memset(first2chars, 0, 3);
    strncpy(first2chars, response, 2);

    if (strcmp(first2chars, "OK") == 0) {
        return true;   
    }

    return false;
}

int os_connect (char *name) {
    sprintf(objstore_errstr, "KO Not a valid name\n");
	if (!name) return false;
	if (strlen(name) < 1) return false;
	//If we haven't connected yet
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

    //Send message
    dprintf(objstore_fd, "REGISTER %s \n", name);

    //Get and return response
    size_t recv_len = recv(objstore_fd, buff, ERRSTR_LEN, 0);
    if (recv_len <= 0) {
        if (recv_len < 0) err_read(objstore_fd);
        return false;
    }

    return check_response((char*)buff);
}

void *os_retrieve(char *name) {
    sprintf(objstore_errstr, "KO Not a valid name\n");
	if (!name) return NULL;
	if (strlen(name) < 1) return NULL;
    //Fail if we haven't connected yet
    if (objstore_fd < 0) return NULL;

    dprintf(objstore_fd, "RETRIEVE %s \n", name);

    //Init buff for recv
    char buff[SO_READ_BUFFSIZE];
    memset(buff, 0, SO_READ_BUFFSIZE);

    char saveptr[SO_READ_BUFFSIZE];
    memset(saveptr, 0, SO_READ_BUFFSIZE);
    
    //Receive data
    size_t buffsize = recv(objstore_fd, buff, SO_READ_BUFFSIZE, 0);
    
    //Length of "OK \n" is 4 + null terminator = 5
    char isok[5];   
    memset(isok, 0, 5);
    strncpy(isok, buff, 4);

    //Oops something went wrong with retrieving name
    if (strcmp(isok, "DATA") != 0) {
        check_response(buff);
        return NULL;
    } 

    //Get length of data that has been sent
    char *storecmd = strtok_r(buff, " ", (char**)&saveptr);
    char *lenstr = strtok_r(NULL, " ", (char**)&saveptr);
    size_t len = atol(lenstr);
    
    char *data = (char*)calloc(len, sizeof(char));

    //Length of "DATA len \n "
    size_t headerlen = strlen(storecmd) + 1 + strlen(lenstr) + 3;   
    
    //If there's some data to write
    if (headerlen < buffsize) memcpy(data, buff + headerlen, buffsize - headerlen);

    //buffsize now contains how much data we have read from the first recv
    buffsize = buffsize - headerlen;

    //If there's MORE data to write
    if (len > buffsize) {
        ssize_t readsz = buffsize;
        while(readsz < len) {
            size_t readbytes = recv(objstore_fd, data + readsz, SO_READ_BUFFSIZE, 0);
            readsz = readsz + readbytes;
        }
    }
    
    LAST_LENGTH = len;
    
    return (void*)data;
}

int os_store(char *name, void *block, size_t len) {
    sprintf(objstore_errstr, "KO Not a valid name\n");
	if (!name) return false;
	if (strlen(name) < 1) return false;
	if (objstore_fd < 0) return false;
    if (!block || len < 1) return false;

    //Init header
    char header[512];
    memset(header, 0, 512);
    sprintf((char*)header, "STORE %s %ld \n ", name, len);

    ssize_t headerlen = strlen(header);

    //Init server message
    char *tosend = (char*)calloc(headerlen + len, sizeof(char));

    //Write header
    memcpy(tosend, header, headerlen);
    
    //Write rest of the data and send it
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
    sprintf(objstore_errstr, "KO Not a valid name\n");
	if (!name) return false;
	if (strlen(name) < 1) return false;
	if (objstore_fd < 0) return false;
    dprintf(objstore_fd, "DELETE %s \n", name);

    char buff[ERRSTR_LEN];
    memset(buff, 0, ERRSTR_LEN);
    
    size_t recv_len = recv(objstore_fd, buff, ERRSTR_LEN, 0);
    if (recv_len <= 0) {
        if (recv_len < 0) err_read(objstore_fd);
        return false;
    }

    return check_response(buff);
}

int os_disconnect() {
    if (objstore_fd < 0) {
        sprintf(objstore_errstr, "KO You're not registered\n");
        return false;
    }
    dprintf(objstore_fd, "LEAVE \n");

    char recv_ok[5];
    memset(recv_ok, 0, 5);   
    recv(objstore_fd, recv_ok, 5, 0);

    //Close socket connection
    close(objstore_fd);
    objstore_fd = -1;
    
    //Restore SIGPIPE
    sighandler_t result = signal(SIGPIPE, prevsignal);
    if (result == SIG_ERR) {
        err_signal()
        return false;
    }

    return check_response(recv_ok);
}
