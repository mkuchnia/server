#include "clientService.hpp"
#include <iostream>
#include <arpa/inet.h> //close
#include <unistd.h> //close
#include <string.h> //strlen

#define TRUE 1
#define FALSE 0

using namespace std;

void disconnect(int socketDescriptor)
{
	struct sockaddr_in address;
	int addrlen;
	//Somebody disconnected , get his details and print
	getpeername(socketDescriptor, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
	
	//Close the socket
	close(socketDescriptor);
}

void client_service(int socketDescriptor)
{
	fd_set socketSet;
	int valueReaded;
	int activity;
	char buffer[1025];
	bool END = FALSE;

	//clear the set
	FD_ZERO(&socketSet);
	//add socket to set
	FD_SET(socketDescriptor, &socketSet);

	while (!END)
	{
		//wait for an activity on the socket , timeout is set
		struct timeval tv = { 10, 0 };
		activity = select(socketDescriptor + 1, &socketSet, NULL, NULL, &tv);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		if (activity == 0)
		{
			//cout << "No activity on sd: " << socketDescriptor << endl;
			disconnect(socketDescriptor);
			END = TRUE;
		}

		if (FD_ISSET(socketDescriptor, &socketSet))
		{
			if ((valueReaded = read(socketDescriptor, buffer, 1024)) == 0)
			{
				//cout << "Disconnected sd: " << socketDescriptor << endl;
				disconnect(socketDescriptor);
				END = TRUE;
			}
			else
			{
				//set the string terminating NULL byte on the end
				//of the data read
				buffer[valueReaded] = '\0';
				printf("%s", buffer);
				send(socketDescriptor, buffer, strlen(buffer), 0);
			}
		}
	}
}