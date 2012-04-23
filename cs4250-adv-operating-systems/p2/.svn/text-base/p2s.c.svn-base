#include "p2.h"

#define DEFAULT_BACKLOG 10

int daemonize();
int get_bound_server_socket(u_int32_t addr_num, u_int16_t port);
void receive_and_set_command_limits(int client_sock);
char** receive_command_args(int client_sock);
void receive_and_send_sig(int signum);
int serve_on_socket(int sockfd, int sigsockfd);
void start_servlet(int client_sock, int client_sig_sock);


int clsigsock; //for reading in sig handler
pid_t servlet_child_pid;

int main(int argc, char** argv)
{
	int i;
	int port = -1;
	int daemon = 0;

	if(argc < 3){
		printf("Usage: %s [-p port] [-d]\n", argv[0]);
		printf("\t-d: make server a daemon process\n");
		exit(-1);
	}
	for(i = 0; i < argc; i++){
		if(strcmp(argv[i], "-d") == 0)
			daemon = 1;
		else if((strcmp(argv[i], "-p") == 0) && argv[i+1]){
			port = atoi(argv[i+1]);
		}
	}	

	if(port == -1){
		fprintf(stderr, "Missing mandatory [-p port] argument.\n");
		exit(-1);
	}
	if(port < 1024){
		fprintf(stderr, "Error: port must be number above 1024!\n");
		exit(-1);
	}

	if(daemon)
		daemonize();


	serve_on_socket( get_bound_server_socket(INADDR_ANY, htons(port)),
			 get_bound_server_socket(INADDR_ANY, htons(port+1)) );
	exit(0);
}

int get_bound_server_socket(u_int32_t addr_num, u_int16_t port)
{
	int serv_sock;
	struct sockaddr_in sin;
	char addr_str[INET_ADDRSTRLEN];

	sin.sin_family = AF_INET;
	sin.sin_port = port;
	sin.sin_addr.s_addr = addr_num;


	if((serv_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		err_die("error getting server socket");

	if(bind(serv_sock, (struct sockaddr*)&sin, sizeof(sin)) < 0)
		err_die("error binding server socket");

	debug("bound on address %s port %d\n",
		inet_ntop(sin.sin_family, &sin.sin_addr, 
			  addr_str, INET_ADDRSTRLEN),
		ntohs(port));

	return serv_sock;
}

int serve_on_socket(int sockfd, int sigsockfd)
{
	int client_sock, client_sig_sock;
	char cl_addr_str[INET_ADDRSTRLEN];
	struct sockaddr_in cl_sin, cl_sig_sin;
	socklen_t cl_addr_len = sizeof(cl_sin);
	socklen_t cl_sigaddr_len = sizeof(cl_sig_sin);
	pid_t pid;


	if(listen(sockfd, DEFAULT_BACKLOG) < 0)
		err_die("error listening on data socket");
	if(listen(sigsockfd, DEFAULT_BACKLOG) < 0)
		err_die("error listening on signal socket");
	

	while(1){

		if((client_sock = accept(sockfd, 
				         (struct sockaddr*)&cl_sin, 
				         &cl_addr_len)) < 0)
			err_die("error accepting connection on data channel");


		debug("server: accepted connection from %s on port %d\n",
			inet_ntop(cl_sin.sin_family, &cl_sin.sin_addr,
				  cl_addr_str, INET_ADDRSTRLEN),
			ntohs(cl_sin.sin_port));


		// Accept connection on signal socket
		if((client_sig_sock = accept(sigsockfd,
					     (struct sockaddr*)&cl_sig_sin,
					     &cl_sigaddr_len)) < 0)
			err_die("error accepting connection on signal channel");

		debug("server: accepted connection from %s on port %d\n",
			inet_ntop(cl_sig_sin.sin_family, &cl_sig_sin.sin_addr,
				  cl_addr_str, INET_ADDRSTRLEN),
			ntohs(cl_sig_sin.sin_port));



		if((pid = fork()) < 0)
			err_die("error forking servlet");
		if(pid == 0){
			close(sockfd);
			close(sigsockfd);
			debug("server: starting servlet...\n");
			start_servlet(client_sock, client_sig_sock);
			close(client_sock);
			close(client_sig_sock);
			exit(0);
		}else{
			close(client_sock);
			close(client_sig_sock);
			waitpid(-1, NULL, WNOHANG); //(limited) zombie control
			debug("server: ready for new client\n");
		}
	}
}

void start_servlet(int client_sock, int client_sig_sock)
{
	pid_t pid;
	int child_status, i, waitrc, killsig;
	const size_t BUF_SIZE = 64;
	char buf[BUF_SIZE];
	char** exec_args;
	

	if((pid = fork()) < 0)
		err_die("error forking process");
	if(pid == 0){
		//process creates new process group
		setpgrp();
		exec_args = receive_command_args(client_sock);
		//receive_and_set_command_limits(client_sock);

		if(dup2(client_sock, STDOUT_FILENO) != STDOUT_FILENO ||
		   dup2(client_sock, STDIN_FILENO) != STDIN_FILENO ||
		   dup2(client_sock, STDERR_FILENO) != STDERR_FILENO)
		   	err_die("error attaching socket in servlet");

		close(client_sock);	
		close(client_sig_sock);
		execvp(exec_args[0], exec_args);
		err_die("error executing servlet program");

	}else{
		if(fcntl(client_sig_sock, F_SETOWN, getpid()) < 0)
			err_die("server:error setting owner for signal socket");
		if(fcntl(client_sig_sock, F_SETFL, O_ASYNC) < 0)
			err_die("server:error setting flags for signal socket");

		//Set up signal handling information.
		clsigsock = client_sig_sock;
		servlet_child_pid = pid;
		setsigfunc(SIGIO, receive_and_send_sig);


		if(waitpid(pid, &child_status, 0) < 0)
			err_die("server: error waiting for process");

		sprintf(buf, "Process on server returned status %d\n",
			child_status);
		if(write_full(client_sock, buf, strlen(buf)+1) < 0)
			err_die("server: error writing exit status to socket");

	}
}

void receive_and_set_command_limits(int client_sock)
{
	int cpu_limit;
	long mem_limit;
	struct rlimit rlim;

	if(read_full(client_sock, &mem_limit, sizeof(long)) != sizeof(long))
		err_die("error recieving command mem limit");
	if(read_full(client_sock, &cpu_limit, sizeof(int)) != sizeof(int))
		err_die("error recieving command cpu limit");
	
	debug("server: mem_limit = %d, cpu_limit = %d\n", 
			mem_limit, cpu_limit);

	if(getrlimit(RLIMIT_AS, &rlim) < 0)
		err_die("error getting current mem limit");

	rlim.rlim_cur = mem_limit;
	if(setrlimit(RLIMIT_AS, &rlim) < 0)
		err_die("error setting new mem limit");

	if(getrlimit(RLIMIT_CPU, &rlim) < 0)
		err_die("error getting current cpu limit");

	rlim.rlim_cur = cpu_limit;
	if(setrlimit(RLIMIT_CPU, &rlim) < 0)
		err_die("error setting new cpu limit");
}

char** receive_command_args(int client_sock)
{
	int arg_number, arg_size, i;
	char *args, *next_arg, **exec_args;

	if(read_full(client_sock, &arg_number, sizeof(int)) != sizeof(int))
		err_die("error recieving command arg number");
	if(read_full(client_sock, &arg_size, sizeof(int)) != sizeof(int))
		err_die("error recieving command arg size");

	//debug("server: arg_num = %d, arg_size = %d\n", arg_number, arg_size);


	args = (char*)malloc(arg_size);
	if(read_full(client_sock, args, arg_size) != arg_size)
		err_die("error recieving command args");

	exec_args = (char**)malloc(sizeof(char*) * (arg_number+1));

	next_arg = args;
	for(i = 0; i < arg_number; i++){
		exec_args[i] = next_arg;
		next_arg += ((strlen(next_arg)+1) * sizeof(char));
	}
	exec_args[arg_number] = 0;

	return exec_args;
}

void receive_and_send_sig(int signum)
{
	int rc;
	int cl_signum;
	debug("server: caught SIGIO for signal socket \n");

	if((rc = read_full(clsigsock, &cl_signum, sizeof(int))) > 0){
		//kill process group of servlet child
		if(kill((servlet_child_pid*-1), cl_signum) < 0)
			perror("server: unable to send signal to process");

		debug("server: sent signal %d to process %d\n", 
			cl_signum, 
			servlet_child_pid);
	} 
	else if (rc < 0) {
		err_die("server: error reading socket in signal handler");
	}
}

int daemonize()
{
    if ( fork() != 0 )  /* parent exits; child in background */
	exit( 0 );

    setsid();  /* become session leader; no controlling tty */

    /* some svr4-specific things */
    /* leader exits; make sure do not get another controlling tty; */
    /* when the session leader terminates, all processes in the
       foreground process group are sent a SIGHUP signal (apparently)
    */
    signal( SIGHUP,SIG_IGN );
    if ( fork() != 0 )  
	exit( 0 );

    chdir("/"); /* free up filesys for umount */
    umask(0);   /* clear umask so I can set permissions I want */

    freopen( "/dev/null", "a", stdout );
    freopen( "/dev/null", "a", stderr );

    close( 0 );
}
