#ifndef OS_FS_H

    void fs_init();
    int fs_mkdir(const char *name);
    int fs_write(int cfd, const char *name, char *filename, size_t len, char *data, size_t datalen);
    int fs_delete(const char *name, char *filename);
    int fs_read(int cfd, const char *name, char *filename);

    #define OS_FS_H

#endif