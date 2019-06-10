#ifndef OS_ERROR_MACROS_H

	#define err_open(filename) \
		{char buf[256]; \
		strerror_r(errno, buf, 256); \
	    fprintf(stderr, "Error opening file \'%s\': %s\n", filename, buf);} \

	#define err_write(filename) \
		{char buf[256]; \
		strerror_r(errno, buf, 256); \
	    fprintf(stderr, "Error writing to file \'%s\': %s\n", filename, buf);} \

	#define err_read(filename) \
		{char buf[256]; \
		strerror_r(errno, buf, 256); \
	    fprintf(stderr, "Error reading from file \'%s\': %s\n", filename, buf);} \
	
	#define err_close(filename) \
		{char buf[256]; \
		strerror_r(errno, buf, 256); \
	    fprintf(stderr, "Error closing file '%s\': %s\n", filename, buf);} \

	#define err_malloc(size) \
		{char buf[256]; \
		strerror_r(errno, buf, 256); \
	    fprintf(stderr, "Error malloc'ing %zu bytes: %s\n", size, buf);}

	#define OS_ERROR_MACROS_H

#endif

