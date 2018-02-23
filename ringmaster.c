#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <limits>
#include "potato.h"
using namespace std;

#define START_PORT 40000

void closeConnect (indexnode* id2node, int socket_fd, struct addrinfo* host_info_list, int joined, int *acc2recv) {
	ringnode termination;
	termination.metahead.type = IS_CLOSE;
	for (int i = 0; i < joined; i++)
	{
		//cout << "send IS_CLOSE to node: " << i << endl;
		send(id2node[i].client_connection_fd, &termination, sizeof(ringnode), 0);
		close(id2node[i].client_connection_fd);
	}
	freeaddrinfo(host_info_list);
	close(socket_fd);
	delete[] id2node;
	delete[] acc2recv;
}

void confirmNet(indexnode* id2node, int joined,  int num_players, ringnode *ptr, int client_connection_fd) { 
	id2node[joined].ringhead = *ptr;
	id2node[joined].ringhead.id = joined;
	id2node[joined].ringhead.num_players = num_players;
	id2node[joined].client_connection_fd = client_connection_fd;
	id2node[joined].ringhead.metahead.type = IS_NET;
	//cout << "host name: " << ptr->name << "  id: " << joined << endl;

	if(joined == 0) {
	//	cout << "send IS_NET to node: " << joined <<endl;
		send(client_connection_fd, &(id2node[joined].ringhead), sizeof(ringnode), 0);
	} else {
		ringnode confirm = id2node[joined - 1].ringhead;
		confirm.id = joined;
		confirm.metahead.type = IS_NET;
	//	cout << "send IS_NET to node: " << joined << endl;
		send(client_connection_fd, &confirm, sizeof(ringnode), 0);
		if(joined == num_players - 1) {
			confirm = id2node[joined].ringhead;
			confirm.id = 0;
			confirm.metahead.type = IS_CONNECT;
		//	cout << "send IS_CONNECT to node: 0" << joined << endl;
			send(id2node[0].client_connection_fd, &confirm, sizeof(ringnode), 0 );
		}
	}
	
}

int main (int argc, char* argv[]) {
	// check if input is valid or not
	if(argc != 4) {
		cerr << "Usage: ./ringmaster <port_num> <num_players> <num_hops>" << endl;
		return 0;
	}
	int port_num = atoi(argv[1]);
	int num_players = atoi(argv[2]);
	int num_hops = atoi(argv[3]);
	if( port_num < 0 || port_num > 65535 || num_hops > 512 || num_hops < 0 || num_players < 1) {
		cerr << "Invalid input for number of port or hops or players!" << endl;
		return 0;
	}

	cout << "Potato Ringmaster" << endl;
	cout << "Player = " << num_players << endl;
	cout << "Hops = " << num_hops << endl;

	//initialize map for recording the topology of net
	indexnode* id2node = new indexnode[num_players];
	int trace[512];
	int *accept_order_fd = new int[num_players];
	int *acc2recv = new int[num_players];

	// open socket for listening
	struct addrinfo *host_info_list;
	struct addrinfo host_info;
	int socket_fd = set_server(&host_info_list, argv[1]);
	struct sockaddr_storage socket_addr;
	socklen_t socket_addr_len = sizeof(socket_addr);
	
	int max_sd = -1, joined = 0, confirmed = 0, ready = 0, potato_count = 0, activity;
	while(joined < num_players) {
		accept_order_fd[joined] =  accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
		joined++;
	}


	fd_set readfds;
	while (true) {
		//clear socket set
		max_sd = -1;
		FD_ZERO(&readfds);
		/*if(joined < num_players ) {
			FD_SET(socket_fd, &readfds);
			max_sd = socket_fd;
		}*/
		for(int i = 0; i < joined; i++ ) {
			int sd = accept_order_fd[i];
			if ( sd > 0)
				FD_SET (sd, &readfds);
			max_sd = sd > max_sd ? sd : max_sd;
		}
		//cout << "wait for select" <<endl;
		activity = select (max_sd + 1, &readfds, NULL, NULL, NULL);
		//cout << "finish select activity: " << activity << " " <<endl;
		/*if(joined < num_players && FD_ISSET(socket_fd, &readfds)) {
			id2node[joined].client_connection_fd =  accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
			joined++;
			activity--;
		} */
		for(int i = 0 ; i < joined /*&& activity > 0*/ ; i++ ) {
			//cout << i << " fd checking now" << endl;
			int sd = accept_order_fd[i];
			if(FD_ISSET(sd, &readfds)) {
				//cout << i << " fd ISSET: ";
				char buffer[512];
				recv(sd, buffer, sizeof(ringnode), 0);
				ringnode* meta_ptr = (ringnode*)buffer;
				if(confirmed < num_players) { //管理recv的顺序
					if(meta_ptr->metahead.type == IS_NET) {
						//cout << "IS_NET from " << i << " " <<endl;
						acc2recv[i] = confirmed;
						confirmNet(id2node, confirmed , num_players , meta_ptr, accept_order_fd[i] ); // 只加一，并把包读出来放到数组中，不回复，等到所有都收到了再一个个回复
						confirmed++;
					} 
				} 
				if(ready < num_players) { 
					if(meta_ptr->metahead.type == IS_CONFIRM_CONNECT ) {
						//cout << "IS_CONFIRM_CONNECT from " << i << " " <<endl;
						cout << "Player " << acc2recv[i] << " is ready to play" << endl;
						ready++;
						if (ready == num_players)
						{
							if(num_hops == 0) {
								closeConnect(id2node, socket_fd, host_info_list, joined, acc2recv);
								exit(EXIT_SUCCESS);
							}
							srand((unsigned int)time(NULL));
							int random = rand() % num_players;
							ringnode _potato;
							_potato.hops = num_hops;
							_potato.num_hops = num_hops;
							_potato.metahead.type = IS_POTATO;
							_potato.num_players = num_players;
							_potato.id = random;
							//cout << "send IS_POTATO to node:" << random << endl;
							cout << "Ready to start the game, sending potato to player " << random << endl;
							send(id2node[random].client_connection_fd, &_potato, sizeof(ringnode), 0);
						}
					}
				} else  { 
					if(meta_ptr->metahead.type == IS_POTATO ){
						potato_count++;
						ringnode *potato_ptr = (ringnode *)meta_ptr;
						trace[num_hops - potato_ptr->hops - 1] = potato_ptr->id; 
						if (potato_count == num_hops)
						{
							cout << "Trace of potato:" << endl;
							for (int i = 0; i < num_hops; i++)
							{
								cout << trace[i];
								if (i < num_hops - 1)
									cout << ",";
								else
									cout << endl;
							}
							closeConnect(id2node, socket_fd, host_info_list, joined, acc2recv);
							exit(EXIT_SUCCESS);
						}
					}
				}
			}
		}
	}
}