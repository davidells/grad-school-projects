#include <arpa/inet.h>
#include <errno.h>
#include <malloc.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEBUG 0

void 
err_die(const char* message){
	perror(message);
	exit(-1);
}

void debug(const char* message, ...){
	#if DEBUG == 1
		va_list	printf_args;
		va_start(printf_args, message);
		vfprintf(stderr, message, printf_args);
		va_end(printf_args);
	#endif
}

size_t read_full(int fd, void *buf, size_t size){
	ssize_t n; 
	size_t total = 0;

	while((n = read(fd, buf, size)) > 0){
		if((total += n) == size)
			return total;
	}
	return n;
}	

size_t write_full(int fd, void *buf, size_t size){
	ssize_t n; 
	size_t total = 0;

	while((n = write(fd, buf, size)) > 0){
		if((total += n) == size)
			return total;
	}
	return n;
}
