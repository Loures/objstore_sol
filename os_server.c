#include <os_server.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dispatcher.h>
#include <fs.h>

#define SOCKET_ADDR "/tmp/objstore.sock"

sigset_t nosignal;
linkedlist_elem *client_list = NULL;

size_t SO_READ_BUFFSIZE = 0;
size_t SO_WRITE_BUFFSIZE = 0;

volatile sig_atomic_t OS_RUNNING = 1;
volatile sig_atomic_t PRINT_STATS = 0;
static sigset_t sigmask;
static int waited_sig = 0;
int os_serverfd;
int VERBOSE = 0;
volatile int worker_num = 0;
pthread_mutex_t client_list_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t worker_num_cond = PTHREAD_COND_INITIALIZER;

static void _handler(int sig) {
	switch (sig) {
		case SIGTERM:
			if (VERBOSE) {
				fprintf(stderr, "OBJSTORE: Received SIGTERM\n");
			}
			//linkedlist_free(client_list);
			OS_RUNNING = 0;
			
			break;

		case SIGINT:
			if (VERBOSE) {
				fprintf(stderr, "OBJSTORE: Received SIGINT\n");
			}
			//linkedlist_free(client_list);
			OS_RUNNING = 0;
			break;
		
		case SIGUSR1:
			PRINT_STATS = 1;
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
	int err = sigfillset(&nosignal);
	if (err < 0) fprintf(stderr, "OBJSTORE: Error filling sigset\n");
	pthread_sigmask(SIG_BLOCK, &nosignal, NULL);	//block all signals

	VERBOSE = getopt(argc, argv, "v") > 0;

	/*SERVER SOCKET HANDLING AND DISPATCHER THREAD CREATION*/
	
    os_serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (os_serverfd < 0) err_socket(os_serverfd);

	ssize_t size = sizeof(size_t);
	getsockopt(os_serverfd, SOL_SOCKET, SO_RCVBUF, (void*)&SO_READ_BUFFSIZE, (socklen_t*)&size);
	getsockopt(os_serverfd, SOL_SOCKET, SO_SNDBUF, (void*)&SO_WRITE_BUFFSIZE, (socklen_t*)&size);

    if (VERBOSE) { 
        fprintf(stderr, "OBJSTORE: Server socket created with fd %d\n", os_serverfd); 
        fprintf(stderr, "OBJSTORE: Socket read buffer size is %ld bytes\n", SO_READ_BUFFSIZE); 
        fprintf(stderr, "OBJSTORE: Socket write buffer size is %ld bytes\n", SO_WRITE_BUFFSIZE); 
    }

    struct sockaddr_un socket_address;
    memset(&socket_address, 0, sizeof(struct sockaddr_un));     //set the struct to zero and fill it
    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, SOCKET_ADDR, sizeof(socket_address.sun_path) - 1); 
    
    err = unlink(SOCKET_ADDR);     //might have previous socket from SIGKILL'd process
	if (!err) {
		fprintf(stderr, "OBJSTORE: Socket already exists. Terminating execution\n");
		return 1;
	}
    
	err = bind(os_serverfd, (const struct sockaddr*)&socket_address, sizeof(socket_address));      //bind the socket
    if (err < 0) err_socket(os_serverfd);
    
    if (VERBOSE) { 
        fprintf(stderr, "OBJSTORE: Server socket bound to %s\n", socket_address.sun_path); 
    }

    setnonblocking(os_serverfd);   //non blocking socket
    listen(os_serverfd, SOMAXCONN);    //listen mode

	fs_init();

	pthread_create(&dispatcher_thread, NULL, &dispatch, NULL);
	
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGUSR1);
	sigwait(&sigmask, &waited_sig);
	_handler(waited_sig);

	pthread_join(dispatcher_thread, NULL);

}
