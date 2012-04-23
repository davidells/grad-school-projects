#include "p1.h"

struct sockaddr_in* get_host_inet_addr(const char* host);
void send_command_limits(int conn_sock, long mem_limit, int cpu_limit);
void send_command_args(int conn_sock, int argc, char** argv, int arg_start);
void enter_communication_loop(int conn_sock);

int main(int argc, char** argv)
{
	int conn_sock, port, i, arg_size;
	int cpu_limit;
	long mem_limit;
	struct sockaddr_in *server_addr;
	char addr_str[INET_ADDRSTRLEN];


	if(argc < 6){
		printf("Usage: %s [host] [port] ", argv[0]);
		printf("[mem limit (mb)] [time limit (s)] ");
		printf("[command] [args]\n");
		exit(-1);
	}
	if((port = atoi(argv[2])) < 1024){
		fprintf(stderr, "Port must be number higher than 1024!\n");
		exit(-1);
	}
	if((mem_limit = atol(argv[3])) <= 0){
		fprintf(stderr, "Memory limit must be a positive number!\n");
		exit(-1);
	}
	if((cpu_limit = atoi(argv[4])) <= 0){
		fprintf(stderr, "CPU time limit must be a positive number!\n");
		exit(-1);
	}

	if((server_addr = get_host_inet_addr(argv[1])) == NULL){
		fprintf(stderr, "Error finding host address. Exiting.\n");
		exit(-1);
	}

	server_addr->sin_port = htons(port);


	debug("client: connecting to address %s port %d\n",
		inet_ntop(server_addr->sin_family, &server_addr->sin_addr, 
			  addr_str, INET_ADDRSTRLEN), port); 


	if((conn_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_die("error getting socket");

	if(connect(conn_sock, 
		   (struct sockaddr*)server_addr, 
		   sizeof(*server_addr)) < 0)
		err_die("error connecting to server");

	debug("client: connected successfully\n");


	mem_limit *= 1024; //kilobytes
	mem_limit *= 1024; //megabytes

	send_command_args(conn_sock, argc, argv, 5);
	send_command_limits(conn_sock, mem_limit, cpu_limit);
	enter_communication_loop(conn_sock);
	close(conn_sock);

	exit(0);
}

struct sockaddr_in* get_host_inet_addr(const char* host)
{
	int err;
	struct addrinfo hint;
	struct addrinfo *ai_list, *ai_entry;
	struct sockaddr *addr;
	socklen_t inet_addrlen = sizeof(struct sockaddr_in);

	hint.ai_flags = AI_ADDRCONFIG;
	hint.ai_family = AF_INET;
	hint.ai_socktype = 0;
	hint.ai_protocol = 0;
	hint.ai_addrlen = 0;
	hint.ai_addr = NULL;
	hint.ai_canonname = NULL;
	hint.ai_next = NULL;

	if((err = getaddrinfo(host, NULL, &hint, &ai_list)) != 0){
		fprintf(stderr, "error looking up host : %s\n", 
				gai_strerror(err));
		exit(-1);
	}

	ai_entry = ai_list;
	while(ai_entry != NULL){
		addr = ai_entry->ai_addr;
		if(addr != NULL && sizeof(*addr) == inet_addrlen){
			return (struct sockaddr_in*)addr;
		}
		ai_entry = ai_list->ai_next;
	}
	return NULL;
}

void send_command_limits(int conn_sock, long mem_limit, int cpu_limit)
{
	debug("client: mem_limit = %d, cpu_limit = %d\n", mem_limit, cpu_limit);

	if(write_full(conn_sock, &mem_limit, sizeof(long)) != sizeof(long))
		err_die("error writing mem limit to socket");
	if(write_full(conn_sock, &cpu_limit, sizeof(int)) != sizeof(int))
		err_die("error writing cpu limit to socket");
}

void send_command_args(int conn_sock, int argc, char** argv, int arg_start)
{
	int i;
	int arg_size = 0;
	int arg_number = argc - arg_start;


	for(i = arg_start; i < argc; i++)
		arg_size += (strlen(argv[i])+1);

	debug("client: arg_num = %d, arg_size = %d\n", arg_number, arg_size);


	if(write_full(conn_sock, &arg_number, sizeof(int)) != sizeof(int))
		err_die("error writing arg number to socket");

	if(write_full(conn_sock, &arg_size, sizeof(int)) != sizeof(int))
		err_die("error writing args size to socket");

	if(write_full(conn_sock, argv[arg_start], arg_size) != arg_size)
		err_die("error writing args to socket");
}

void enter_communication_loop(int conn_sock)
{
	ssize_t n;
	size_t BUF_SIZE = 4096;
	char buf[BUF_SIZE];

	fd_set readers;
	FD_ZERO(&readers);
	FD_SET(STDIN_FILENO, &readers);
	FD_SET(conn_sock, &readers);


	while(select(conn_sock+1, &readers, NULL, NULL, NULL) > 0){
		if(FD_ISSET(conn_sock, &readers)){
			if((n = read(conn_sock, buf, BUF_SIZE)) > 0){
				if(write_full(STDOUT_FILENO, buf, n) < 0)
					err_die("error writing to stdout");
			}
			if(n < 0)
				err_die("error reading from socket");
			if(n == 0)
				break;
		}
		if(FD_ISSET(STDIN_FILENO, &readers)){
			if((n = read(STDIN_FILENO, buf, BUF_SIZE)) > 0){
				if(write_full(conn_sock, buf, n) < 0)
					err_die("error writing to socket");
			}
			if(n < 0)
				err_die("error reading from stdin");
			if(n == 0)
				shutdown(conn_sock, SHUT_WR);
		}
		FD_ZERO(&readers);
		FD_SET(STDIN_FILENO, &readers);
		FD_SET(conn_sock, &readers);
	}
}

