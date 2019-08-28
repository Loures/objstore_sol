#ifndef OS_ERROR_MACROS_H

	#define err_open(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error opening file \'%s\': %s\n", filename, buf);} \

	#define err_mkdir(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error creating directory \'%s\': %s\n", filename, buf);} \

	#define err_write(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error writing to file descriptor %d: %s\n", fd, buf);} \

	#define err_read(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error reading from file descriptor %d: %s\n", fd, buf);} \
	
	#define err_close(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error closing file descriptor %d: %s\n", fd, buf);} \

	#define err_malloc(size) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error allocating %ld bytes: %s\n", size, buf);}

	#define err_signal() \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error setting sigmask: %s\n", buf);}
	
	#define err_socket(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error on socket fd %d: %s\n", fd, buf);}

	#define err_unlink(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error removing file \'%s\': %s\n", filename, buf);}

	#define err_select(fd) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error polling fd %d: %s\n", fd, buf);}

	#define err_pipe() \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "Error creating pipe: %s\n", buf);}
	
	#define err_accept() \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "accept() Error: %s\n", buf);}

	#define err_ftw(filename) \
		{char buf[128]; \
		strerror_r(errno, buf, 128); \
	    fprintf(stderr, "ftw() Error on \'%s\' folder: %s\n", filename, buf);}


	#define OS_ERROR_MACROS_H

#endif

