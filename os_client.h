#define OS_CLIENT_REGISTER 0
#define OS_CLIENT_STORE    1
#define OS_CLIENT_RETRIEVE 2
#define OS_CLIENT_DELETE   3
#define OS_CLIENT_LEAVE    4

typedef struct os_msg_t {
	char *cmd;
	char *name;
	size_t len;
	char *data;
} os_msg_t;

extern int os_client_commandhandler(int fd, client_t *client, os_msg_t *msg);