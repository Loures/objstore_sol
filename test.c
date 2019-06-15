#include <os_server.h>
#include <objstore.h>

const char *reg = "REGISTER %s \n";
const char *leave = "LEAVE \n";

void receive_reply(int cfd, char *buff) {
	memset(buff, 0, 256);
	recv(cfd, buff, 256, 0);
	printf(buff);
}

int main(int argc, char *argv[]) {

	os_connect("marco");
	char *buff = os_retrieve("video.mp4");
	int fd = open(argv[1], O_CREAT | O_RDWR, S_IRWXU);
	write(fd, buff, LAST_LENGTH);

	return 0;
}

