#include <os_server.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <dispatcher.h>
#include <fs.h>

sigset_t nosignal;
ht_t *client_list = NULL;

size_t SO_READ_BUFFSIZE = 0;

int os_signalfd[2];

volatile sig_atomic_t OS_RUNNING = 1;
static sigset_t sigmask;
static int wait_sig = 0;
int os_serverfd;
int VERBOSE = 0;
volatile int worker_num = 0;
pthread_mutex_t worker_num_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t worker_num_cond = PTHREAD_COND_INITIALIZER;

pthread_t dispatcher_thread;

const char *one = "1";

static void sighandler(int sig) {
	int err;
	switch (sig) {
		case SIGTERM:
			if (VERBOSE) fprintf(stderr, "OBJSTORE: Received SIGTERM\n");
			
			OS_RUNNING = 0;
			break;

		case SIGINT:
			if (VERBOSE) fprintf(stderr, "OBJSTORE: Received SIGINT\n");
			
			OS_RUNNING = 0;
			break;
		
		case SIGUSR1:
			//See "self-pipe trick"
			err = write(os_signalfd[1], one, 1);
			if (err < 0) err_write(os_signalfd[1]);

			//SIGUSR1 doesnt terminate the process so we keep waiting for another signal
			sigemptyset(&sigmask);			
			sigaddset(&sigmask, SIGINT);
			sigaddset(&sigmask, SIGTERM);
			sigaddset(&sigmask, SIGUSR1);
			sigwait(&sigmask, &wait_sig);
			sighandler(wait_sig);

		default:
			break;
	}
}

int main(int argc, char *argv[]) {
	int err = sigfillset(&nosignal);
	if (err < 0) fprintf(stderr, "OBJSTORE: Error filling sigset\n");
	
	//Block all signals;
	pthread_sigmask(SIG_BLOCK, &nosignal, NULL);

	//Running os_server -v prints lots of info about what's going on
	VERBOSE = getopt(argc, argv, "v") > 0;

	//SERVER SOCKET HANDLING AND DISPATCHER THREAD CREATION
	
    os_serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (os_serverfd < 0) err_socket(os_serverfd);

	//Get size of kernel socket read buffer
	ssize_t size = sizeof(size_t);
	getsockopt(os_serverfd, SOL_SOCKET, SO_RCVBUF, (void*)&SO_READ_BUFFSIZE, (socklen_t*)&size);

    if (VERBOSE) { 
        fprintf(stderr, "OBJSTORE: Server socket created with fd %d\n", os_serverfd); 
        fprintf(stderr, "OBJSTORE: Socket read buffer size is %ld bytes\n", SO_READ_BUFFSIZE); 
    }

	//Init socket_address struct
    struct sockaddr_un socket_address;
    memset(&socket_address, 0, sizeof(struct sockaddr_un));     
    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, SOCKET_ADDR, sizeof(socket_address.sun_path) - 1);		//-1 so we don't exceed max path size
    
	//There might be another os_server process running or we might still have a previous socket from a killed os_server process
	struct stat sb;
	if (stat(SOCKET_ADDR, &sb) == 0) {
		fprintf(stderr, "OBJSTORE: Socket already exists. Terminating execution\n");
		exit(EXIT_FAILURE);
	}

	//Bind the socket
	err = bind(os_serverfd, (const struct sockaddr*)&socket_address, sizeof(socket_address));      
    if (err < 0) err_socket(os_serverfd);
    
    if (VERBOSE) fprintf(stderr, "OBJSTORE: Server socket bound to %s\n", socket_address.sun_path); 

	//Start listening for connections and keep a SOMAXCONN-sized backlog
    listen(os_serverfd, SOMAXCONN);

	//Init fs module (i.e create data folder)
	fs_init();

	//Init client_list
	client_list = (ht_t*)calloc(1, sizeof(ht_t));
	myhash_init(client_list, HASHTABLE_SIZE, HASHTABLE_LOCKS);
	
	//Create pipe for self-pipe trick
	err = pipe(os_signalfd);

	//Create dispatcher thread
	pthread_create(&dispatcher_thread, NULL, &dispatch, NULL);
	
	//Wait for SIGINT, SIGTERM, SIGUSR1 and discard all the others
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGUSR1);
	sigwait(&sigmask, &wait_sig);

	sighandler(wait_sig);

	//Wait for dispatcher thread to finish, this command gets executed only after a SIGINT or SIGTERM signal (see sigwait)
	pthread_join(dispatcher_thread, NULL);

	exit(EXIT_SUCCESS);

}
