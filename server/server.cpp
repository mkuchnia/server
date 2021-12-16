//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux
#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRUE 1
#define FALSE 0
#define PORT 8888

#include <iostream>
using namespace std;

#include <boost/thread.hpp>

void client_service(int sd)
{
	cout << "TEST sd: " << sd << endl;\

	fd_set readfds;
	int valread;
	int activity;
	char buffer[1025];
	bool END = FALSE;

	while (!END)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(sd, &readfds);

		////wait for an activity on one of the sockets , timeout is NULL ,
		////so wait indefinitely
		//wait 1sec
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

		activity = select(sd + 1, &readfds, NULL, NULL, &tv);

		if (activity == 0)
		{
			cout << "No activity on sd: " << sd << endl;
			struct sockaddr_in address;
			int addrlen;
			//Somebody disconnected , get his details and print
			getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
			printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
			//Close the socket and mark as 0 in list for reuse
			close(sd);
			END = TRUE;
		}

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		if (FD_ISSET(sd, &readfds))
		{
			if ((valread = read(sd, buffer, 1024)) == 0)
			{
				struct sockaddr_in address;
				int addrlen;
				//Somebody disconnected , get his details and print
				getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
				printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
				//Close the socket and mark as 0 in list for reuse
				close(sd);
				END = TRUE;
			}
			else
			{
				//set the string terminating NULL byte on the end
				//of the data read
				buffer[valread] = '\0';
				printf("%s", buffer);
				send(sd, buffer, strlen(buffer), 0);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int opt = TRUE;
	int master_socket, addrlen, new_socket, activity, i, valread, sd;
	int max_sd;
	struct sockaddr_in address;

	char buffer[1025]; //data buffer of 1K

	//set of socket descriptors
	fd_set readfds;

	//a message
	char* message = "ECHO Daemon v1.0 \r\n";

	//create a master socket
	if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//set master socket to allow multiple connections ,
	//this is just a good habit, it will work without this
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

		////wait for an activity on one of the sockets , timeout is NULL ,
		////so wait indefinitely
		//wait 1sec
		struct timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		activity = select(master_socket + 1, &readfds, NULL, NULL, &tv);

		if (activity == 0)
		{
			cout << "No activity" << endl;
		}

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}

		//If something happened on the master socket ,
		//then its an incoming connection
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

			//send new connection greeting message
			if (send(new_socket, message, strlen(message), 0) != strlen(message))
			{
				perror("send");
			}

			puts("Welcome message sent successfully");

			//add new socket to array of sockets
			boost::thread t(&client_service, new_socket);
		}

		//else its some IO operation on some other socket
		//for (i = 0; i < max_clients; i++)
		//{
		//	sd = client_socket[i];

		//	if (FD_ISSET(sd, &readfds))
		//	{
		//		//Check if it was for closing , and also read the
		//		//incoming message
		//		if ((valread = read(sd, buffer, 1024)) == 0)
		//		{
		//			//Somebody disconnected , get his details and print
		//			getpeername(sd, (struct sockaddr*)&address, \
		//				(socklen_t*)&addrlen);
		//			printf("Host disconnected , ip %s , port %d \n",
		//				inet_ntoa(address.sin_addr), ntohs(address.sin_port));

		//			//Close the socket and mark as 0 in list for reuse
		//			close(sd);
		//			client_socket[i] = 0;
		//		}

		//		//Echo back the message that came in
		//		else
		//		{
		//			//set the string terminating NULL byte on the end
		//			//of the data read
		//			buffer[valread] = '\0';
		//			printf("%s", buffer);
		//			send(sd, buffer, strlen(buffer), 0);
		//		}
		//	}
		//}
	}

	return 0;
}
