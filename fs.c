#include <os_server.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fs.h>

const char *DATA_PATH = "data";
const char *retrieve = "DATA ";

void fs_init() {
    int err = mkdir(DATA_PATH, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir("data") 
    else {
        if (VERBOSE && errno != EEXIST) fprintf(stderr, "OBJSTORE: Data directory created\n");
    }
}

int fs_mkdir(client_t *client) {
    char path[strlen(DATA_PATH) + strlen(client->name) + 2]; //slash and null terminator    
    sprintf(path, "%s/%s", DATA_PATH, client->name);
    int err = mkdir(path, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir(path);
    return 0;
}

int fs_write(int cfd, client_t *client, char *filename, size_t len, char *data, size_t datalen) {
    char path[strlen(DATA_PATH) + strlen(client->name) + strlen(filename) + 3]; //3 for slashes (/) and null terminator
    sprintf(path, "%s/%s/%s", DATA_PATH, client->name, filename);
    int fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        err_open(path);
        return -1;
    }
    ssize_t wlen = write(fd, data, datalen);   
    if (wlen < 0) {
        err_write(fd);
        return -1;
    }
    ssize_t wrotelen = wlen;
    if (wrotelen < len) {
        char buff[SO_READ_BUFFSIZE];
        while (wrotelen < len) {
            memset(buff, 0, SO_READ_BUFFSIZE);
            ssize_t bufflen = recv(cfd, buff, SO_READ_BUFFSIZE, 0);
            ssize_t wlen = pwrite(fd, buff, bufflen, wrotelen);
            wrotelen = wrotelen + wlen;
        }
    }
    close(fd);
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Wrote %ld bytes to file \"%s\"\n", wrotelen, path);
    return 0;
}

int fs_delete(client_t *client, char *filename) {
    char path[strlen(DATA_PATH) + strlen(client->name) + strlen(filename) + 3]; //3 for slashes (/) and null terminator
    sprintf(path, "%s/%s/%s", DATA_PATH, client->name, filename);
    int err = unlink(path);
    if (err < 0) {
        err_unlink(filename);
        return -1;
    }
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Deleted file \"%s\"\n", path);
    return 0;
}

int fs_read(int cfd, client_t *client, char *filename) {
    char path[strlen(DATA_PATH) + strlen(client->name) + strlen(filename) + 3]; //3 for slashes (/) and null terminator
    sprintf(path, "%s/%s/%s", DATA_PATH, client->name, filename);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        err_open(path);
        return 0;
    }
    struct stat statbuf;
    fstat(fd, &statbuf);
    char buff[statbuf.st_size];
    ssize_t readlen = read(fd, (char*)buff, (ssize_t)(statbuf.st_size));
    if (readlen < 0) {
        err_write(fd);
        return 0;
    }
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Read %ld bytes from file \"%s\"\n", readlen, path);
    close(fd);

    char len[sizeof(ssize_t) + 1];
    memset(len, 0, sizeof(ssize_t) + 1);
    sprintf(len, "%ld", readlen);
    ssize_t response_len = strlen(retrieve) + strlen(len) + 3 + readlen;
    char *response = (char*)calloc(response_len, sizeof(char));   //3 -> space newline space
    sprintf(response, "DATA %s \n ", len);
    memcpy(response + (strlen(retrieve) + strlen(len) + 3), buff, readlen);
    send(cfd, (char*)response, response_len, 0);
    free(response);
    
    return 1;
    //remember to free dis shit;
}

