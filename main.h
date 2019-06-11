#ifndef OS_MAIN_H

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <signal.h>
	#include <linkedlist.h>


	extern sigset_t nosignal;	
	extern volatile sig_atomic_t OS_RUNNING;
	
	typedef struct client_t {
		char *name;
		int socketfd;
		pthread_t worker;
	} client_t;

	extern linkedlist_elem* client_list;

	#define SOCKET_ADDR "/tmp/objstore.sock"
	#define SOMAXCONN 128

	#define discardsignals(sgn) \
		{ \
			int err = sigfillset(&sgn); \
			if (err < 0) fprintf(stderr, "Error emptying sigset\n"); \
		} \

		
	#define OS_MAIN_H
	
#endif
