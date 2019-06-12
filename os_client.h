#define OS_CLIENT_REGISTER 0
#define OS_CLIENT_STORE    1
#define OS_CLIENT_RETRIEVE 2
#define OS_CLIENT_DELETE   3
#define OS_CLIENT_LEAVE    4

extern int os_client_commandhandler(client_t *client, int fd, char *header, size_t headerlen);