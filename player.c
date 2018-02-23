#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include "potato.h"

using namespace std;

void diliverPotato (ringnode* node_ptr, int* socket_0rin1se2le3rig, int id ) 
{
//	cout <<"potato is from" << node_ptr->id << " hops: " << node_ptr->hops << endl;
	node_ptr->hops--;
	node_ptr->id = id;
	if (node_ptr->hops > 0)
	{	
		//cout << "send potato to neighbor" << endl;
		int random = rand();
		//cout << "random is " << random << " % 2 = " << random % 2 << " ";
		random %= 2; 
		int targetid =  ( (random == 0 ? id - 1  : id + 1)   + node_ptr->num_players ) % node_ptr->num_players;
		cout << "Sending potato to " <<  targetid << endl;
		send(socket_0rin1se2le3rig[2 + random], node_ptr, sizeof(ringnode), 0); // randomly select left or right
		send(socket_0rin1se2le3rig[0], node_ptr, sizeof(ringnode), 0);
	} else if (node_ptr->hops == 0){ 
		cout << "I’m it" << endl;
		send(socket_0rin1se2le3rig[0], node_ptr, sizeof(ringnode), 0);
	}
	//cout << "send potato to ringmaster" << endl;
	
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		cerr << "Usage: ./player <machine_name> <port_num>" << endl;
		exit(EXIT_FAILURE);
	}
	int port_num = atoi(argv[2]);
	if (port_num < 0 || port_num > 65535)
	{
		cerr << "Invalid input for number of port !" << endl;
		exit(EXIT_FAILURE);
	}
	int socket_0rin1se2le3rig[4] = {-1,-1,-1,-1};
	struct addrinfo *host_info_list[4];
	struct addrinfo host_info[4];

	struct sockaddr_storage socket_addr;
	socklen_t socket_addr_len = sizeof(socket_addr);
	/////////////////////////open a random valid port////////////////////////////
	//char port[6] = "0";
	socket_0rin1se2le3rig[1] = set_server( &(host_info_list[1]),  NULL);
	
	////////////////////////open a soket connect to ringmaster///////////////////
	socket_0rin1se2le3rig[0] = set_client( &(host_info[0]), &(host_info_list[0]), argv[1], argv[2]);
	
	srand((unsigned int)time(NULL));
	indexnode mynode;
	mynode.ringhead.metahead.type = IS_NET;
	mynode.ringhead.metahead.size = sizeof(mynode);
	gethostname(mynode.ringhead.name, 512);
	struct hostent* h;
	h = gethostbyname(mynode.ringhead.name);
	strcpy(mynode.ringhead.name, h->h_name);

	struct sockaddr_in sock_info;
	socklen_t length_sock_info = sizeof(sock_info);
	if(getsockname(socket_0rin1se2le3rig[1], (struct sockaddr*)&sock_info, &length_sock_info) == -1)
		cerr << "getsockname error" << endl;
	int port_num_listen = (int) ntohs(sock_info.sin_port);
	snprintf(mynode.ringhead.port, 6, "%d", port_num_listen);

	//cout<< "open port " << mynode.ringhead.port << "for right to connect, need to send message to ringmaster" << endl; 
	send(socket_0rin1se2le3rig[0], &(mynode.ringhead), sizeof(ringnode), 0);
	//cout<< "successfully send message to ringmaster" << endl; 
	int max_sd = -1, hasID = 0;
	fd_set readfds;
	while(true) {
		max_sd = -1;
		FD_ZERO(&readfds);
		for(int i = 0 ; i < 4 ; i++) {
			if (socket_0rin1se2le3rig[i] > -1 ) {
				FD_SET( socket_0rin1se2le3rig[i] ,&readfds);
				max_sd = max(socket_0rin1se2le3rig[i], max_sd);
			}
		}
		char buffer[512];	
		//cout<< "wait for select" << endl; 
		int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
		//cout << "finish select activity:"  << activity << " " <<endl;
		if(hasID == 0) { // didn't receive hasID， need IS_NET,player 0 need IS_CONNECT
			//cout << "not ready for potato" << endl;
			if( FD_ISSET(socket_0rin1se2le3rig[0], &readfds) ) {
				//cout << "message from ringmaster, it is a: " ;
				
				recv(socket_0rin1se2le3rig[0], buffer, sizeof(ringnode), 0);
				ringnode* node_ptr = (ringnode*) buffer;
				
				if(node_ptr->metahead.type == IS_NET) {
					//cout << "IS_NET" << endl;
					cout << "Connected as player " << node_ptr->id << " out of " << node_ptr->num_players <<" total players"<< endl;
					mynode.ringhead.num_players = node_ptr->num_players;
					mynode.ringhead.id = node_ptr->id;
					if(mynode.ringhead.id == 0) {
						socket_0rin1se2le3rig[3] =  accept(socket_0rin1se2le3rig[1], (struct sockaddr *)&socket_addr, &socket_addr_len);
					} else {
						socket_0rin1se2le3rig[2] = set_client(&(host_info[2]),&(host_info_list[2]), node_ptr->name, node_ptr->port);
						socket_0rin1se2le3rig[3] =  accept(socket_0rin1se2le3rig[1], (struct sockaddr *)&socket_addr, &socket_addr_len);
						ringnode confirm = mynode.ringhead;
						confirm.metahead.type = IS_CONFIRM_CONNECT;
						//cout<< "send IS_CONFIRM_CONNECT to ringmaster" << endl; 
						send(socket_0rin1se2le3rig[0], &confirm, sizeof(ringnode), 0); // ready
						hasID = 1;
					}
				} else if (node_ptr->metahead.type == IS_CONNECT) {
					//cout << "IS_CONNECT" << endl;
					if(mynode.ringhead.id != 0 ) {
						cerr << "Error: only player 0 can receive IS_CONNECT" << endl;
						exit(EXIT_FAILURE);
					}
					socket_0rin1se2le3rig[2] = set_client(&(host_info[2]),&(host_info_list[2]), node_ptr->name, node_ptr->port);
					ringnode connect = mynode.ringhead;
					connect.metahead.type = IS_CONFIRM_CONNECT;
					//cout<< "send IS_CONFIRM_CONNECT to ringmaster" << endl;
					send(socket_0rin1se2le3rig[0], &connect, sizeof(ringnode), 0); // ready
					hasID = 1;
				}
			}
		}  else { 
			//cout << "ready for potato" << endl;
			if(FD_ISSET( socket_0rin1se2le3rig[2], &readfds) ) {
				//cout << "message from left " ;
				recv(socket_0rin1se2le3rig[2], buffer, sizeof(ringnode), 0);
				ringnode* node_ptr = (ringnode*) buffer;
				if (node_ptr->metahead.type == IS_POTATO && node_ptr->id == ( mynode.ringhead.id - 1 + mynode.ringhead.num_players) % mynode.ringhead.num_players )
				{
					diliverPotato (node_ptr, socket_0rin1se2le3rig, mynode.ringhead.id ) ;
				}
			} 
			if(FD_ISSET(socket_0rin1se2le3rig[3], &readfds)) {
				//cout << "message from right " ;
				recv(socket_0rin1se2le3rig[3], buffer, sizeof(ringnode), 0);
				ringnode* node_ptr = (ringnode*) buffer;
				if (node_ptr->metahead.type == IS_POTATO && node_ptr->id ==  (mynode.ringhead.id + 1 + mynode.ringhead.num_players) % mynode.ringhead.num_players )
				{
					diliverPotato (node_ptr, socket_0rin1se2le3rig, mynode.ringhead.id ) ;
				}
			}
			if(FD_ISSET(socket_0rin1se2le3rig[0], &readfds) ) {
				//cout << "message from ringmaster, it is a: " ;
				recv(socket_0rin1se2le3rig[0], buffer, sizeof(ringnode), 0);
				ringnode* node_ptr = (ringnode*) buffer;
				//cout << "type " << node_ptr->metahead.type <<endl;
				if(node_ptr->metahead.type == IS_POTATO && node_ptr->id == mynode.ringhead.id ) {
					//cout << "IS_POTATO" << endl;
					diliverPotato (node_ptr, socket_0rin1se2le3rig , mynode.ringhead.id) ;
				} else if(node_ptr->metahead.type == IS_CLOSE){
					//cout << "IS_CLOSE" << endl;
					for(int i = 3; i >= 0 ; i--) {
						if(i != 3) {
							freeaddrinfo(host_info_list[i]);
						}
						close(socket_0rin1se2le3rig[i]);
					}
					exit(EXIT_SUCCESS);
				}
			}
		}
		
	}
}