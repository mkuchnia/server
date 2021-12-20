#include "clientService.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>

#define TRUE 1
#define FALSE 0

using namespace std;

void disconnect(int socketDescriptor)
{
	int addrlen;
	struct sockaddr_in address;
	//somebody disconnected , get his details and print
	getpeername(socketDescriptor, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
	//close the socket
	close(socketDescriptor);
}

void client_service(int socketDescriptor)
{
	long int connectionStartTime = time(0);
	connectionNumber++;

	bool END = FALSE;
	char buffer[1025];
	int activity, valueReaded;
	fd_set socketSet;

	//clear the set
	FD_ZERO(&socketSet);
	//add socket to set
	FD_SET(socketDescriptor, &socketSet);

	while (!END)
	{
		//wait for an activity on the socket, timeout is set
		struct timeval tv = { 30, 0 };
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
					else if (obj["cmd"] == "SLEEP")
					{
						//answer
						newObj["status"] = "ok";
						int delay = obj["args"]["delay"].asInt();
						//action
						sleep(delay);
						//answer
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else if (obj["cmd"] == "WRITE")
					{
						//action
						string key = obj["args"]["key"].asString();
						string value = obj["args"]["value"].asString();
						MYSQL mysql;
						mysql_init(&mysql);

						if (mysql_real_connect(&mysql, "127.0.0.1", "root", "password", "B", 0, NULL, 0))
						{
							//printf("Connected with MySQL\n");
							//save logs
							string query = "INSERT INTO logs (cmd, `key`, val_before, val_after) VALUES ('WRITE', '" + key + "', (SELECT `value` FROM A.`data` WHERE A.`data`.`key` = '" + key + "'), '" + value + "'); ";
							mysql_query(&mysql, query.c_str());
							//save data
							mysql_select_db(&mysql, "A");
							query = "REPLACE INTO data VALUES ('" + key + "', '" + value + "');";
							mysql_query(&mysql, query.c_str());
							//answer
							newObj["status"] = "ok";
							mysql_close(&mysql);
						}
						else
						{
							printf("MySQL error: %d, %s\n", mysql_errno(&mysql), mysql_error(&mysql));
							//answer
							newObj["status"] = "error";
						}
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else if (obj["cmd"] == "READ")
					{
						//action
						string key = obj["args"]["key"].asString();
						MYSQL mysql;
						mysql_init(&mysql);

						if (mysql_real_connect(&mysql, "127.0.0.1", "root", "password", "B", 0, NULL, 0))
						{
							//printf("Connected with MySQL\n");
							//save logs
							string query = "INSERT INTO logs (cmd, `key`, val_before, val_after) VALUES ('READ', '" + key + "', (SELECT `value` FROM A.`data` WHERE A.`data`.`key` = '" + key + "'), (SELECT `value` FROM A.`data` WHERE A.`data`.`key` = '" + key + "')); ";
							mysql_query(&mysql, query.c_str());
							//read data
							query = "SELECT `value` FROM A.`data` WHERE A.`data`.`key` = '" + key + "';";
							mysql_query(&mysql, query.c_str());
							MYSQL_RES* result = mysql_store_result(&mysql);
							MYSQL_ROW row = mysql_fetch_row(result);
							string value = "";
							if (row != NULL)
							{
								value = row[0];
							}
							//answer
							newObj["status"] = "ok";
							newObj["value"] = value;
							mysql_close(&mysql);
						}
						else
						{
							printf("MySQL error: %d, %s\n", mysql_errno(&mysql), mysql_error(&mysql));
							//answer
							newObj["status"] = "error";
						}
						output = fastWriter.write(newObj);
						send(socketDescriptor, output.c_str(), strlen(output.c_str()), 0);
					}
					else if (obj["cmd"] == "DEL")
					{
					//action
					string key = obj["args"]["key"].asString();
					MYSQL mysql;
					mysql_init(&mysql);

					if (mysql_real_connect(&mysql, "127.0.0.1", "root", "password", "B", 0, NULL, 0))
					{
						//printf("Connected with MySQL\n");
						//save logs
						string query = "INSERT INTO logs (cmd, `key`, val_before, val_after) VALUES ('DEL', '" + key + "', (SELECT `value` FROM A.`data` WHERE A.`data`.`key` = '" + key + "'), NULL); ";
						mysql_query(&mysql, query.c_str());
						//delete data
						query = "DELETE FROM A.`data` WHERE A.`data`.`key` = '" + key + "';";
						mysql_query(&mysql, query.c_str());
						//answer
						newObj["status"] = "ok";
						mysql_close(&mysql);
					}
					else
					{
						printf("MySQL error: %d, %s\n", mysql_errno(&mysql), mysql_error(&mysql));
						//answer
						newObj["status"] = "error";
					}
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