#include "clientService.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <jsoncpp/json/json.h>

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
	long int connectionStartTime = time(0);
	connectionNumber++;
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
		//wait for an activity on the socket, timeout is set
		struct timeval tv = { 10, 0 };
		activity = select(socketDescriptor + 1, &socketSet, NULL, NULL, &tv);

		if ((activity < 0) && (errno != EINTR))
		{
			printf("select error");
		}
		else if (activity == 0)
		{
			//cout << "No activity on sd: " << socketDescriptor << endl;
			disconnect(socketDescriptor);
			END = TRUE;
		}
		else if (FD_ISSET(socketDescriptor, &socketSet))
		{
			if ((valueReaded = read(socketDescriptor, buffer, 1024)) == 0)
			{
				//cout << "Disconnected sd: " << socketDescriptor << endl;
				disconnect(socketDescriptor);
				END = TRUE;
			}
			else
			{
				//handle the command
				queryNumber++;
				//set the string terminating NULL byte on the end of the data read
				buffer[valueReaded] = '\0';
				//parse to JSON
				Json::Reader reader;
				Json::Value obj;
				reader.parse(buffer, obj);
				//cout << obj << endl;

				Json::Value newObj;
				Json::StyledWriter fastWriter;
				string output;

				try
				{
					if (obj["cmd"] == "END")
					{
						//answer
						newObj["status"] = "ok";
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
						//action
						disconnect(socketDescriptor);
						END = TRUE;
					}
					else if (obj["cmd"] == "STAT")
					{
						//answer
						newObj["status"] = "ok";
						long int nowTime = time(0);
						int serverRuntime = nowTime - serverStartTime;
						newObj["serverRuntime"] = serverRuntime;
						int connectionTime = nowTime - connectionStartTime;
						newObj["connectionTime"] = connectionTime;
						newObj["queryNumber"] = queryNumber.load();
						newObj["connectionNumber"] = connectionNumber.load();
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else if (obj["cmd"] == "INC")
					{
						//answer
						newObj["status"] = "ok";
						long long number = obj["args"]["number"].asInt64();
						//action
						int hits;
						mtxIncrementMap.lock();
						incrementMap[number]++;
						hits = incrementMap[number];
						mtxIncrementMap.unlock();
						//answer
						newObj["hits"] = hits;
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else if (obj["cmd"] == "GET")
					{
						//answer
						newObj["status"] = "ok";
						long long number = obj["args"]["number"].asInt64();
						//action
						int hits;
						mtxIncrementMap.lock();
						hits = incrementMap[number];
						mtxIncrementMap.unlock();
						//answer
						newObj["hits"] = hits;
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else
					{
						//answer
						newObj["status"] = "error";
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
				}
				catch (...) 
				{
					//answer
					newObj["status"] = "error";
					output = fastWriter.write(newObj);
					send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
				}
				
			}
		}
	}
	connectionNumber--;
}