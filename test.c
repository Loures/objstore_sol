#include <os_server.h>
#include <signal.h>
#include <linkedlist.h>
#include <errormacros.h>
#include <sys/socket.h>
#include <sys/un.h>


unsigned int remaining = 12;

void iterator(const void *ptr, void *arg) {
	printf("%d ", *(int*)ptr);
}

void sighandler(int sig) {
	printf("Hello %d\n", remaining);
}

int ll_comp(const void *ptr, void *arg) {
	int n = *(int*)arg;
	return *(int*)ptr % 2;
}

char *inttochar(int from, char *to) {
	int temp = from;
    memcpy(to, &temp, sizeof(int));
    return to;
}

int main(int argc, char *argv[]) {
	signal(SIGUSR1, &sighandler);
	linkedlist_elem *list = NULL;	
	for (int i = 0; i < 25; i++) {
		int *bubu = (int*)malloc(sizeof(int));
		*bubu = i;
		list = linkedlist_new(list, bubu);
	}
	linkedlist_iter(list, &iterator, NULL);
	printf("\n");
	printf("\t%d\n", fcntl(100, F_GETFD));
	int n = 5;
	linkedlist_elem* res;
	while(res = linkedlist_search(list, &ll_comp, &n)) {
		list = linkedlist_remove(list, res);
		linkedlist_iter(list, &iterator, NULL);
		printf("\n");
	}
	linkedlist_free(list);
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	n = 13;
	strncpy(addr.sun_path, SOCKET_ADDR, sizeof(addr.sun_path) - 1);
	char culo[256];
	sprintf(culo, "OK %s\n", "AAA");
	printf("\t%s\n", culo);
	for (int i = 0; i < 10; i++) {
		int cfd = socket(AF_UNIX, SOCK_STREAM, 0);
		int rfd = connect(cfd, (struct sockaddr*)&addr, sizeof(addr));
		if (rfd < 0) err_socket();
		char num[sizeof(int)];
		const char* reg = "REGISTER pepo\n";
		inttochar(strlen(reg), num);
		
		send(cfd, num, sizeof(int), 0);
		send(cfd, reg, strlen(reg), 0);

		recv(cfd, num, sizeof(int), 0);
		char buff[*(int*)num];
		recv(cfd, buff, *(int*)num, 0);
		
		printf(buff);

		const char* leave="LEAVE\n";
		inttochar(strlen(leave), num);
		
		int rs = send(cfd, num, sizeof(int), 0);
		if (rs < 0) err_socket();
		send(cfd, leave, strlen(reg), 0);

		recv(cfd, num, sizeof(int), 0);
		char buff2[*(int*)num];
		recv(cfd, buff2, *(int*)num, 0);
		
		printf(buff2);



		//nanosleep(&((struct timespec){0, 300000000}), NULL);
	}
	sleep(5);
	return 0;
}

