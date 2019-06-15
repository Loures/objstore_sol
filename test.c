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

	os_connect(argv[1]);
	int fd = open(argv[2], O_CREAT | O_RDWR, S_IRWXU);
	char *buff = (char*)malloc(sizeof(char) * 113159810);
	read(fd, buff, 113159810);
	os_store(argv[2], buff, 113159810);
	os_disconnect();

	return 0;
}

