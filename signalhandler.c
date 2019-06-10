#include <main.h>

static sigset_t sigmask;

void os_signalhandler() {
    discardsignals(nosignal);
    pthread_sigmask(SIG_BLOCK, &nosignal, NULL);
	sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGTERM);
    sigaddset(&sigmask, SIGQUIT);
    sigaddset(&sigmask, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
}
