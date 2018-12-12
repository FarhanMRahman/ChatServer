#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string> 
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <vector>
#include <fstream>

using namespace std;

void *receive_msg(void *sock)
{
	int client_sock = *((int *)sock);
	char message[500];
	int length;
	while((length = recv(client_sock, message, 500, 0)) > 0) 
	{
		message[length] = '\0';
		printf("%s\n", message);
		memset(message,'\0',sizeof(message));
	}
}

vector<string> fileToVec(string inputFile) {
	ifstream stream;
	vector<string> lines;
	string line;
	while(getline(stream, line)) {
		lines.push_back(line);
	}
	return lines;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in server;
	char message[500];
	char username[100];
	char message2[600];
	char ip[INET_ADDRSTRLEN];
	pthread_t client_thread;
	int length;
	int client_sock;
	int port;
	vector<string> inputLines;

	if(argc > 4) {
		printf("Too many arguments");
		exit(1);
	}

	if(argc == 4) {
		string inputFile = argv[3];
		inputLines = fileToVec(inputFile);
		printf("Using input file for commands.");
	}
	port = atoi(argv[2]);
	strcpy(username,argv[1]);
	client_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(server.sin_zero,'\0',sizeof(server.sin_zero));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	char user[200];
	strcpy(user, username);
	strcat(user, ":NEW");

	if(connect(client_sock,(struct sockaddr *)&server,sizeof(server)) < 0) {
		perror("You have no connection not esatablished");
		exit(1);
	}
	else write(client_sock,user,strlen(user));

	inet_ntop(AF_INET, (struct sockaddr *)&server, ip, INET_ADDRSTRLEN);
	printf("You are connected to %s, you can now start chatting\n",ip);
	pthread_create(&client_thread, NULL, receive_msg, &client_sock);
	if(argc < 4) {
		while(*(fgets(message, 500, stdin)) > 0) {
			strcpy(message2, username);
			strcat(message2 ,":");
			strcat(message2 ,message);
			length = write(client_sock, message2, strlen(message2));
			if(length < 0) 
			{
				perror("Message not sent");
				exit(1);
			}
			memset(message,'\0',sizeof(message));
			memset(message2,'\0',sizeof(message2));
		}
	}
	else {
		for(int i = 0; i < inputLines.size(); i++) {
			strcpy(message2, username);
			strcat(message2 ,":");
			strcat(message2, inputLines.at(i).c_str());
			length = write(client_sock, message2, strlen(message2));
			if(length < 0) 
			{
				perror("Message not sent");
				exit(1);
			}
			memset(message,'\0',sizeof(message));
			memset(message2,'\0',sizeof(message2));
		}
	}
	pthread_join(client_thread, NULL);
	close(client_sock);

}