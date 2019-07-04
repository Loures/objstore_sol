#ifndef OS_SERVER_H

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <signal.h>
	#include <myhash.h>
	#include <errormacros.h>


	extern volatile sig_atomic_t OS_RUNNING;
	
	extern int os_serverfd;
	extern int os_signalfd[];
	
	extern int VERBOSE;

	extern size_t SO_READ_BUFFSIZE;

	extern volatile int worker_num;
	extern pthread_mutex_t worker_num_mtx;
	extern pthread_cond_t worker_num_cond;
	
	
	typedef struct client_t {
		char *name;
		int socketfd;
		int running;
		pthread_t worker;
	} client_t;

	extern ht_t *client_list;

	#define HASHTABLE_SIZE 2048
	#define HASHTABLE_LOCKS 256
	
	#define SOCKET_ADDR "/tmp/objstore.sock"

	#define discardsignals(sgn) \
		{ \
			int err = sigfillset(&sgn); \
			if (err < 0) fprintf(stderr, "OBJSTORE: Error emptying sigset\n"); \
		} \

	#define free_os_msg(msg) \
		{ \
			free(msg->data); \
			free(msg->name); \
			free(msg->cmd); \
			free(msg); \
		} \
	

	#define setnonblocking(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)
	#define setblocking(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK)
		
	#define OS_SERVER_H
	
#endif
