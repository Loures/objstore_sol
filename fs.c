#include <os_server.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fs.h>

const char *DATA_PATH = "data";
const char *retrieve = "DATA ";

//Create the data folder
void fs_init() {
    int err = mkdir(DATA_PATH, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir("data") 
    else {
        if (VERBOSE && errno != EEXIST) fprintf(stderr, "OBJSTORE: Data directory created\n");
    }
}

//Make the client's data folder
int fs_mkdir(const char *name) {
    //+2 to account for slash and null terminator
    char path[strlen(DATA_PATH) + strlen(name) + 2];     
    sprintf(path, "%s/%s", DATA_PATH, name);
    int err = mkdir(path, S_IRWXU | S_IRGRP | S_IROTH);    //rwxr--r--
    if (err < 0 && errno != EEXIST) err_mkdir(path);
    return 1;
}

int fs_write(int cfd, const char *name, char *filename, size_t len, char *data, size_t datalen) {
    //+3 to account for slash and null terminator
    char path[strlen(DATA_PATH) + strlen(name) + strlen(filename) + 3];
    sprintf(path, "%s/%s/%s", DATA_PATH, name, filename);

    //Remove previous filename file
    int err = unlink(path);
    if (err < 0 && errno != ENOENT) {
        err_unlink(path);
        return 0;
    }

    int fd = open(path, O_CREAT | O_RDWR | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        err_open(path);
        return 0;
    }

    //Write the data we got from the os_msg_t struct
    ssize_t wlen = write(fd, data, datalen);   
    if (wlen < 0) {
        err_write(fd);
        return 0;
    }
    ssize_t wrotelen = wlen;

    //We got more data to read!
    if (wrotelen < len) {
        char buff[SO_READ_BUFFSIZE];

        //Keep on reading until we read len bytes
        while (wrotelen < len) {
            memset(buff, 0, SO_READ_BUFFSIZE);
            ssize_t bufflen = recv(cfd, buff, SO_READ_BUFFSIZE, 0);
            if (bufflen < 0) err_read(cfd);

            ssize_t wlen = write(fd, buff, bufflen);
            if (wlen < 0) {
                err_write(fd);
                return 0;
            }        
            wrotelen = wrotelen + wlen;
        }
    }

    close(fd);
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Wrote %ld bytes to file \"%s\"\n", wrotelen, path);
    return 1;
}

int fs_delete(const char *name, char *filename) {
    //+3 to account for slashes and null terminator
    char path[strlen(DATA_PATH) + strlen(name) + strlen(filename) + 3];
    sprintf(path, "%s/%s/%s", DATA_PATH, name, filename);

    //Delete filename from client's data folder
    int err = unlink(path);
    if (err < 0) {
        err_unlink(path);
        return 0;
    }

    if (VERBOSE) fprintf(stderr, "OBJSTORE: Deleted file \"%s\"\n", path);
    return 1;
}

int fs_read(int cfd, const char *name, char *filename) {
    //+3 to account for slashes and null terminator
    char path[strlen(DATA_PATH) + strlen(name) + strlen(filename) + 3];
    sprintf(path, "%s/%s/%s", DATA_PATH, name, filename);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        err_open(path);
        return 0;
    }

    //Get file size in bytes and alloc such size
    struct stat sb;
    fstat(fd, &sb);

    //Convert size_t len to a string
    char len[64];
    memset(len, 0, 64);
    sprintf(len, "%ld", sb.st_size);

    ssize_t response_len = strlen(retrieve) + strlen(len) + 3;
    
    char *response = (char*)calloc(response_len + sb.st_size, sizeof(char));
    if (response == NULL) {
        err_malloc(response_len);
        exit(EXIT_FAILURE);
    } 
    
    size_t readlen = read(fd, response + response_len, (ssize_t)(sb.st_size));
    if (readlen < 0) {
        err_read(fd);
        return 0;
    }
    
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Read %ld bytes from file \"%s\"\n", readlen, path);
    close(fd);

    
    //Length of "DATA len \n " + size of file


    //Setup RETRIEVE response message and send it
    
    char temp[response_len];
    sprintf(temp, "DATA %s \n ", len);
    memcpy(response, temp, response_len);

    ssize_t result = sendn(cfd, (char*)response, response_len + sb.st_size, 0);
    
    if (result < 0) {
        err_write(cfd);
        return 0;
    }

    free(response);

    return 1;   
}

