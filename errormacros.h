#ifndef OS_ERROR_MACROS_H

	#define err_open(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error opening file \'%s\': %s\n", filename, buf);} \

	#define err_mkdir(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error creating directory \'%s\': %s\n", filename, buf);} \

	#define err_write(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error writing to file descriptor %d: %s\n", fd, buf);} \

	#define err_read(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error reading from file descriptor %d: %s\n", fd, buf);} \
	
	#define err_close(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error closing file descriptor %d: %s\n", fd, buf);} \

	#define err_malloc(size) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error malloc'ing %zu bytes: %s\n", size, buf);}

	#define err_signal() \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error setting sigmask: %s\n", buf);}
	
	#define err_socket(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error on server socket %d: %s\n", fd, buf);}

	#define err_unlink(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "EDEBUG: rror removing file \'%s\': %s\n", filename, buf);}

	#define err_select(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "DEBUG: Error polling select on fd %d: %s\n", fd, buf);}


	#define OS_ERROR_MACROS_H

#endif

