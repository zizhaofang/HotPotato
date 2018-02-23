
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>

using namespace std;

#define IS_POTATO 1
#define IS_NET 0
#define IS_CONNECT 3
#define IS_CONFIRM_CONNECT 4
#define IS_CLOSE 5


typedef struct meta {
	int type;
	int size;
} meta;

typedef struct ringnode {
	meta metahead;
	int id;
	int hops;
	int num_players;
	int num_hops;
	char name[256];
	char port[6];
} ringnode;

typedef struct indexnode {
	ringnode ringhead;
	int client_connection_fd;
} indexnode;

int bindUnit( struct addrinfo** host_info_list_ptr, const char* port ) {
	int status;
	int socket_fd;
	struct addrinfo host_info;
	const char *hostname = NULL;

	memset(&host_info, 0, sizeof(host_info));

	host_info.ai_family   = AF_UNSPEC;
	host_info.ai_socktype = SOCK_STREAM;
	host_info.ai_flags    = AI_PASSIVE;

	status = getaddrinfo(hostname, port, &host_info, host_info_list_ptr);
	if (status != 0) {
		return -1;
	} //if

	socket_fd = socket((*host_info_list_ptr)->ai_family, 
				(*host_info_list_ptr)->ai_socktype, 
				(*host_info_list_ptr)->ai_protocol);
	if (socket_fd == -1) {
		return -1 ;
	} //if

	int yes = 1;
	status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


	status = bind(socket_fd, (*host_info_list_ptr)->ai_addr, (*host_info_list_ptr)->ai_addrlen);
	if (status == -1) {
		return -1;
	} //if
	return socket_fd;
}

int set_server ( struct addrinfo** host_info_list_ptr, const char* port) {
	int socket_fd = -1;
	if (port == NULL) {
		for(int i = 51015 ; i <= 51097 && socket_fd == -1; i++) {
			char buf[6] = {0};
			snprintf(buf, 6, "%d", i);
			socket_fd = bindUnit(host_info_list_ptr, buf);
		}
	} else {
		socket_fd = bindUnit(host_info_list_ptr, port);
	}
	if(socket_fd == -1) {
		cerr<<"Error: cannot bind port! " << endl;
		exit(EXIT_FAILURE);
	}
	int status = listen(socket_fd, 100);
	if (status == -1) {
		cerr << "Error: cannot listen on socket" << endl; 
		cerr << "  ( local," << port << ")" << endl;
		exit(EXIT_FAILURE);
	} //if
	return socket_fd;
}

int set_client (struct addrinfo* host_info ,struct addrinfo** host_info_list_ptr, char *hostname, char *port) {
	int status;
	int socket_fd;
	//struct addrinfo host_info;
	struct addrinfo *host_info_list;


	memset(host_info, 0, sizeof(*host_info));
	host_info->ai_family = AF_UNSPEC;
	host_info->ai_socktype = SOCK_STREAM;

	//while( getaddrinfo(hostname, port, host_info, host_info_list_ptr) != 0)
	status = getaddrinfo(hostname, port, host_info, host_info_list_ptr);
	if (status != 0)
	{
		cerr << "Error: cannot get address info for host" << endl;
		cerr << "  (" << hostname << "," << port << ")" << endl;
		exit(EXIT_FAILURE);
	} //if

	//while ( ( socket_fd = socket( (*host_info_list_ptr)->ai_family, (*host_info_list_ptr)->ai_socktype, (*host_info_list_ptr)->ai_protocol)) == -1  );
	socket_fd = socket( (*host_info_list_ptr)->ai_family, (*host_info_list_ptr)->ai_socktype, (*host_info_list_ptr)->ai_protocol);
	if (socket_fd == -1)
	{
		cerr << "Error: cannot create socket" << endl;
		cerr << "  (" << hostname << "," << port << ")" << endl;
		exit(EXIT_FAILURE);
	} 

	//while (   connect(socket_fd, (*host_info_list_ptr)->ai_addr, (*host_info_list_ptr)->ai_addrlen )  == -1 );
	status = connect(socket_fd, (*host_info_list_ptr)->ai_addr, (*host_info_list_ptr)->ai_addrlen ) ;
	if (status == -1)
	{
		cerr << "Error: cannot connect to socket" << endl;
		cerr << "  (" << hostname << "," << port << ")" << endl;
		exit(EXIT_FAILURE);
	} 
	return socket_fd;
}
