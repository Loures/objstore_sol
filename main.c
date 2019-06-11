#include <main.h>
#include <signalhandler.h>
#include <errormacros.h>
#include <dispatcher.h>

sigset_t nosignal;
linkedlist_elem *client_list;
volatile sig_atomic_t OS_RUNNING = 1;
static sigset_t sigmask;
static int sigh = 0;


static void _handler(int sig) {
	#ifdef DEBUG
		if (sig == SIGTERM) fprintf(stderr, "DEBUG: received SIGTERM\n");
		else fprintf(stderr, "DEBUG: received SIGINT\n");
	#endif
    OS_RUNNING = 0;
}

int main(int argc, char *argv[]) {
	discardsignals(nosignal);
	pthread_sigmask(SIG_BLOCK, &nosignal, NULL);
	pthread_create(&dispatcher_thread, NULL, &dispatch, NULL);
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTERM);
	sigwait(&sigmask, &sigh);
	_handler(sigh);
	pthread_join(dispatcher_thread, NULL);
}
