#include <os_server.h>
#include <signal.h>
#include <linkedlist.h>
#include <errormacros.h>
#include <sys/socket.h>
#include <readline/readline.h>
#include <sys/un.h>

const char *reg = "REGISTER %s \n";
const char *leave = "LEAVE \n";

void receive_reply(int cfd, char *buff) {
	memset(buff, 0, 256);
	recv(cfd, buff, 256, 0);
	printf(buff);
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

	char repl[256];

	dprintf(cfd, "REGISTER %s \n", argv[1]);
	receive_reply(cfd, repl);

	int fd = open(argv[2], O_RDONLY);
    
	struct stat statbuf;
    fstat(fd, &statbuf);
	
	char store[256];
	memset(store, 0, 256);

	
	char *rbuff = (char*)malloc(sizeof(char) * statbuf.st_size);
	memset(rbuff, 0, statbuf.st_size);
	ssize_t readz = read(fd, rbuff, statbuf.st_size);
	dprintf(cfd, "STORE %s %ld \n ", argv[2], statbuf.st_size);
	fflush(NULL);
	readz = send(cfd, rbuff, statbuf.st_size, 0);
	receive_reply(cfd, repl);
	dprintf(cfd, "LEAVE \n");
	fflush(NULL);
	printf("DEBUG: Sent %ld\n", readz);

	return 0;
}

