#include <stdio.h>
#include <arpa/inet.h> 
#include <boost/thread.hpp>
#include "clientService.hpp"

#define TRUE 1
#define FALSE 0
#define PORT 8888

using namespace std;

const long int serverStartTime = time(0);
atomic<int> queryNumber(0);
atomic<int> connectionNumber(0);
map<long long, int> incrementMap;
mutex mtxIncrementMap;

int main(int argc, char* argv[])
{
	int activity, addrlen, master_socket, new_socket, opt = TRUE;
	struct sockaddr_in address;
	fd_set readfds;

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections
	if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt,
		sizeof(opt)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	//bind the socket to localhost port 8888
	if (bind(master_socket, (struct sockaddr*)&address, sizeof(address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d \n", PORT);

	//try to specify maximum of 3 pending connections for the master socket
	if (listen(master_socket, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	//accept the incoming connection
	addrlen = sizeof(address);
	puts("Waiting for connections ...");

	while (TRUE)
	{
		//clear the socket set
		FD_ZERO(&readfds);
		//add master socket to set
		FD_SET(master_socket, &readfds);

		//wait for an activity on one of the sockets , timeout is NULL, so wait indefinitely
		activity = select(master_socket + 1, &readfds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		//if something happened on the master socket, then its an incoming connection
		if (FD_ISSET(master_socket, &readfds))
		{
			if ((new_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//inform user of socket number - used in send and receive commands
			printf("New connection , socket fd is %d , ip is : %s , port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs
			(address.sin_port));

			//run thread for new client
			boost::thread t(&client_service, new_socket);
		}
	}
	return 0;
}
