#ifndef OBJSTORE_H

    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <sys/un.h>
    #include <errormacros.h>

	extern size_t LAST_LENGTH;
    extern char objstore_errstr[];

    int os_connect(char *name);

    int os_store(char *name, void *block, size_t len);

    void *os_retrieve(char *name);

    int os_delete(char *name);

    int os_disconnect();

    #define OBJSTORE_H

#endif
