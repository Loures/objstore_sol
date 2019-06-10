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
	#include <bits/types/sigset_t.h>

	extern sigset_t nosignal;	

	#define discardsignals(nosignal) \
		{ \
			int err = sigfillset(&nosignal); \
			if (err < 0) fprintf(stderr, "Error emptying sigset\n"); \
		} \



	#define OS_MAIN_H

#endif
