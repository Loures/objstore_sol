#include <os_server.h>
#include <signal.h>
#include <errormacros.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <sys/un.h>

void receive_reply(int cfd, char *buff) {
	memset(buff, 0, 212992);
	recv(cfd, buff, 212992, 0);
	printf(buff);
}

void parsequery(int fd, char *msg) {
    char saveptr[256];
    char reply_buff[212992];
    memset(saveptr, 0, 256);
    memset(reply_buff, 0, 212992);
    char *cmd = strtok_r(msg, " ", (char**)&saveptr);
    if (!cmd) return;
    if (strcmp(cmd, "REGISTER") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        dprintf(fd, "REGISTER %s \n", name);
        fflush(NULL);
    }
    if (strcmp(cmd, "RETRIEVE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        dprintf(fd, "RETRIEVE %s \n", name);
        fflush(NULL);
    }
    if (strcmp(cmd, "DELETE") == 0) {
        char *name = strtok_r(NULL, " ", (char**)&saveptr);
        dprintf(fd, "DELETE %s \n", name);
        fflush(NULL);

    } else dprintf(fd, msg);
    receive_reply(fd, reply_buff);
    return;
}

int main(int argc, char *argv[]) {
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	strncpy(addr.sun_path, SOCKET_ADDR, sizeof(addr.sun_path) - 1);
	addr.sun_family = AF_UNIX;

	int cfd = socket(AF_UNIX, SOCK_STREAM, 0);
	int rfd = connect(cfd, (struct sockaddr*)&addr, sizeof(addr));
	if (rfd < 0) {
		err_socket(cfd);
		return 1;
	}

    setnonblocking(cfd);

    char *query = readline("> ");
    do {
        if (query) parsequery(cfd, query);
        free(query);
        query = readline("> ");
    } while (query && strcmp(query, "LEAVE") != 0);

    return 0;
}
