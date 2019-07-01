#define OS_CLIENT_STORE    0
#define OS_CLIENT_RETRIEVE 1
#define OS_CLIENT_DELETE   2
#define OS_CLIENT_LEAVE    3

typedef struct os_msg_t {
	char *cmd;
	char *name;
	size_t len;
	size_t datalen;
	char *data;
} os_msg_t;

extern int os_client_commandhandler(int fd, client_t *client, os_msg_t *msg);