#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
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

	while((n = read(fd, buf+total, size-total)) > 0){
		if((total += n) == size)
			return total;
	}
	return n;
}	

size_t write_full(int fd, void *buf, size_t size){
	ssize_t n; 
	size_t total = 0;

	while((n = write(fd, buf+total, size-total)) > 0){
		if((total += n) == size)
			return total;
	}
	return n;
}

int setsigfunc (int signum, void (*sigfunc)(int))
{
	struct sigaction sa;
	sigset_t ss;

	sigemptyset(&ss);
	if(sigaddset(&ss, signum) != 0)
		err_die("client: error calling sigaddset");

	sa.sa_handler = sigfunc;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

#ifdef SA_RESTART
	sa.sa_flags |= SA_RESTART;
#endif
	
	if(sigaction(signum, &sa, NULL) < 0)
		err_die("client: error calling sigaction");

	return 0;
}	
	
