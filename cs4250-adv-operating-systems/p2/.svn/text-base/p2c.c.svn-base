#include "p2.h"
#include <openssl/evp.h>

void enter_communication_loop(int conn_sock);
void get_pass_send_file_digest(int conn_sock, char* file_path);
char* get_password_and_file_hash(char* passwd, char* file_path);
void receive_single_response(int conn_sock);
struct sockaddr_in* get_host_inet_addr(const char* host);
void send_command_limits(int conn_sock, long mem_limit, int cpu_limit);
void send_command_args(int conn_sock, int argc, char** argv, int arg_start);
void send_sig(int signum);


int sig_sock;

int main(int argc, char** argv)
{
	int data_sock, port, i;
	struct sockaddr_in *server_addr;
	char addr_str[INET_ADDRSTRLEN];


	if(argc < 5){
		printf("Usage: %s [filename] [host] [port] ", argv[0]);
		printf("[command] [args]\n");
		exit(-1);
	}
	if((port = atoi(argv[3])) < 1024){
		fprintf(stderr, "Port must be number higher than 1024!\n");
		exit(-1);
	}

	if((server_addr = get_host_inet_addr(argv[2])) == NULL){
		fprintf(stderr, "Error finding host address. Exiting.\n");
		exit(-1);
	}

	server_addr->sin_port = htons(port);


	debug("client: connecting to address %s port %d\n",
		inet_ntop(server_addr->sin_family, &server_addr->sin_addr, 
			  addr_str, INET_ADDRSTRLEN), port); 


	if((data_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_die("error getting data socket");

	if(connect(data_sock, 
		   (struct sockaddr*)server_addr, 
		   sizeof(*server_addr)) < 0)
		err_die("error connecting to server");

	debug("client: connected successfully on data channel\n");


	if((sig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_die("error getting socket");

	//borrow server_addr variable for sig socket, using port+1
	server_addr->sin_port = htons(port+1);
	if(connect(sig_sock, 
		   (struct sockaddr*)server_addr, 
		   sizeof(*server_addr)) < 0)
		err_die("error connecting to server");
	
	debug("client: connected successfully on signal channel\n");
	server_addr->sin_port = htons(port);

	
	setsigfunc(SIGINT, send_sig);
	debug("client: signal set up, catching SIGINT\n");

	send_command_args(data_sock, argc, argv, 4);
	get_pass_send_file_digest(data_sock, argv[1]);
	shutdown(data_sock, SHUT_WR);
	receive_single_response(data_sock);
	//enter_communication_loop(data_sock);
	close(data_sock);
	close(sig_sock);

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

	//debug("client: arg_num = %d, arg_size = %d\n", arg_number, arg_size);


	if(write_full(conn_sock, &arg_number, sizeof(int)) != sizeof(int))
		err_die("error writing arg number to socket");

	if(write_full(conn_sock, &arg_size, sizeof(int)) != sizeof(int))
		err_die("error writing args size to socket");

	if(write_full(conn_sock, argv[arg_start], arg_size) != arg_size)
		err_die("error writing args to socket");
}

void enter_communication_loop(int conn_sock)
{
	int rc;
	ssize_t n;
	size_t BUF_SIZE = 4096;
	char buf[BUF_SIZE];

	fd_set readers;
	FD_ZERO(&readers);
	FD_SET(STDIN_FILENO, &readers);
	FD_SET(conn_sock, &readers);

commloop:
	while((rc = select(conn_sock+1, &readers, NULL, NULL, NULL)) > 0){
		if(FD_ISSET(conn_sock, &readers)){
			if((n = read(conn_sock, buf, BUF_SIZE)) > 0){
				if(write_full(STDOUT_FILENO, buf, n) < 0)
					err_die("client: error writing to stdout");
			}
			if(n < 0)
				err_die("client: error reading from socket");
			if(n == 0)
				break;
		}
		if(FD_ISSET(STDIN_FILENO, &readers)){
			if((n = read(STDIN_FILENO, buf, BUF_SIZE)) > 0){
				if(write_full(conn_sock, buf, n) < 0)
					err_die("client: error writing to socket");
			}
			if(n < 0)
				err_die("client: error reading from stdin");
			if(n == 0)
				shutdown(conn_sock, SHUT_WR);
		}
		FD_ZERO(&readers);
		FD_SET(STDIN_FILENO, &readers);
		FD_SET(conn_sock, &readers);
	}
	if(rc < 0 && errno == EINTR){
		//Interrupted by signal, restart select
		goto commloop;
	}

	debug("client: end of communication loop\n");
}

void send_sig(int signum)
{
	if(write(sig_sock, &signum, sizeof(int)) != sizeof(int))
		err_die("error sending message on signal socket!");
	debug("client: sending signal %d to server\n", signum);
}

void get_pass_send_file_digest(int conn_sock, char* file_path)
{
	char* hash = get_password_and_file_hash(
				getpass("Password:"), 
				file_path);

	if(write_full(conn_sock, hash, strlen(hash)+1) != (strlen(hash)+1))
		err_die("client: error writing hash to socket");
}

char* get_password_and_file_hash(char* passwd, char* file_path)
{
	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE];
        int fileno, md_len, i, bufsize = 1024;
	ssize_t size_read;
	char filebuf[bufsize];
	char *hash_str, *ptr;
    

	if((fileno = open(file_path, O_RDONLY)) < 0)
		err_die("client: could not open file");
	if((size_read = read(fileno, &filebuf, bufsize)) < 0)
		err_die("client: could not read file");
	close(fileno);


	OpenSSL_add_all_digests();
	
    	md = EVP_get_digestbyname("md5");

        EVP_DigestInit(&mdctx, md);
        EVP_DigestUpdate(&mdctx, passwd, strlen(passwd));
        EVP_DigestUpdate(&mdctx, filebuf, size_read);
        EVP_DigestFinal(&mdctx, md_value, &md_len);


	hash_str = malloc(md_len+2);

	ptr = hash_str;
        for(i = 0; i < md_len; i++){
	       	sprintf(ptr, "%02x", md_value[i]);
		ptr += 2;
	}
	*ptr++ = '\n';
	*ptr++ = '\0';

	return hash_str;
}

void receive_single_response(int conn_sock)
{
	ssize_t n;
	size_t BUF_SIZE = 4096;
	char buf[BUF_SIZE];
	

	while((n = read(conn_sock, buf, BUF_SIZE)) > 0){
		if(write_full(STDOUT_FILENO, buf, n) < 0)
			err_die("client: error writing to stdout");
	}
	if(n < 0 && !(errno == ECONNRESET))
		err_die("client: error reading from socket");
}
