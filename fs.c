#include <os_server.h>
#include <errormacros.h>
#include <fs.h>

const char *DATA_PATH = "data";

void fs_init() {
    int err = mkdir(DATA_PATH, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir("data") 
    else {
        #ifdef DEBUG
            fprintf(stderr, "DEBUG: Data directory created\n");
        #endif
    }
}

int fs_mkdir(client_t *client) {
    char path[strlen(DATA_PATH) + strlen(client->name) + 2]; //slash and null terminator    
    sprintf(path, "%s/%s", DATA_PATH, client->name);
    int err = mkdir(path, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir(path);
    return 0;
}

int fs_write(client_t *client, char *filename, size_t len, char *data) {
    char path[strlen(DATA_PATH) + strlen(client->name) + strlen(filename) + 3]; //3 for slashes (/) and null terminator
    sprintf(path, "%s/%s/%s", DATA_PATH, client->name, filename);
    int fd = open(path, O_CREAT | O_RDWR, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd < 0) {
        err_open(path);
        return -1;
    }
    int err = write(fd, data, len);     //+1 for null terminator
    if (err < 0) {
        err_write(fd);
        return -1;
    }
    fsync(fd);      //for consistency, screw buffering!
    close(fd);
    return 0;
}

os_read_t fs_read(client_t *client, char *filename) {
    char path[strlen(DATA_PATH) + strlen(client->name) + strlen(filename) + 3]; //3 for slashes (/) and null terminator
    sprintf(path, "%s/%s/%s", DATA_PATH, client->name, filename);
    printf(filename);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        err_open(path);
        return (os_read_t){NULL, 0};
    }
    struct stat statbuf;
    fstat(fd, &statbuf);
    char *buff = (char*)malloc(sizeof(char) * (ssize_t)(statbuf.st_size));
    int err = read(fd, buff, (ssize_t)(statbuf.st_size));
    if (err < 0) {
        err_write(fd);
        return (os_read_t){NULL, 0};
    }
    return (os_read_t){buff, (ssize_t)(statbuf.st_size)};
    //remember to free dis shit;
}

