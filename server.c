#include <server.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errormacros.h>
#include <dispatcher.h>

sigset_t nosignal;
linkedlist_elem *client_list = NULL;

volatile sig_atomic_t OS_RUNNING = 1;
static sigset_t sigmask;
static int waited_sig = 0;
int serverfd;

static void stats_iterator(const void *ptr, void *arg) {
	client_t *client = (client_t*)ptr;
	fflush(stderr);
	fprintf(stderr, "{NAME: %s, SOCKETFD: %d, WORKER TID: %ld}\n", client->name, client->socketfd, client->worker);
	fflush(stderr);
}

static void _handler(int sig) {
	switch (sig) {
		case SIGTERM:
			#ifdef DEBUG
				fprintf(stderr, "DEBUG: Received SIGTERM\n");
			#endif
			linkedlist_free(client_list);
			OS_RUNNING = 0;
			break;

		case SIGINT:
			#ifdef DEBUG
				fprintf(stderr, "DEBUG: Received SIGINT\n");
			#endif
			linkedlist_free(client_list);
			OS_RUNNING = 0;
			break;
		
		case SIGUSR1:
			linkedlist_iter(client_list, &stats_iterator, NULL);
			sigemptyset(&sigmask);			//SIGUSR1 doesnt terminate the process so we must be able to wait for another signal
			sigaddset(&sigmask, SIGINT);
			sigaddset(&sigmask, SIGTERM);
			sigaddset(&sigmask, SIGUSR1);
			sigwait(&sigmask, &waited_sig);
			_handler(waited_sig);

		default:
			break;
	}
}

int main(int argc, char *argv[]) {
	int err = sigfillset(&nosignal); \
	if (err < 0) fprintf(stderr, "Error filling sigset\n");
	pthread_sigmask(SIG_BLOCK, &nosignal, NULL);	//block all signals

	/*SERVER SOCKET HANDLING AND DISPATCHER THREAD CREATION*/
	
    serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverfd < 0) err_socket();

    #ifdef DEBUG 
        fprintf(stderr, "DEBUG: Server socket created with fd %d\n", serverfd); 
    #endif

    struct sockaddr_un socket_address;
    memset(&socket_address, 0, sizeof(struct sockaddr_un));     //set the struct to zero and fill it
    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, SOCKET_ADDR, sizeof(socket_address.sun_path) - 1); 
    
    err = unlink(SOCKET_ADDR);     //might have previous socket from SIGKILL'd process
	if (!err) {
		fprintf(stderr, "ERROR: Socket already exists. Terminating execution\n");
		return 1;
	}
    
	err = bind(serverfd, (const struct sockaddr*)&socket_address, sizeof(socket_address));      //bind the socket
    if (err < 0) err_socket();
    
    #ifdef DEBUG 
        fprintf(stderr, "DEBUG: Server socket bound to %s\n", socket_address.sun_path); 
    #endif

    setnonblocking(serverfd);   //non blocking socket
    listen(serverfd, SOMAXCONN);    //listen mode

	pthread_create(&dispatcher_thread, NULL, &dispatch, NULL);
	
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGUSR1);
	sigwait(&sigmask, &waited_sig);
	_handler(waited_sig);

	pthread_join(dispatcher_thread, NULL);
}
