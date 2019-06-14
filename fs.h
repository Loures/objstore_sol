#ifndef OS_FS_H

    typedef struct os_read_t {
        char *data;
        ssize_t size;
    } os_read_t;

    void fs_init();
    int fs_mkdir(client_t *client);
    int fs_write(client_t *client, char *filename, size_t len, char *data);
    int fs_delete(client_t *client, char *filename);
    os_read_t fs_read(client_t *client, char *filename);

    #define OS_FS_H

#endif