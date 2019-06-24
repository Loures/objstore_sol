#ifndef OS_FS_H

    void fs_init();
    int fs_mkdir(client_t *client);
    int fs_write(int cfd, client_t *client, char *filename, size_t len, char *data, size_t datalen);
    int fs_delete(client_t *client, char *filename);
    int fs_read(int cfd, client_t *client, char *filename);

    #define OS_FS_H

#endif