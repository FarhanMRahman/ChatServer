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

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

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
	ifstream stream(inputFile);
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
	pthread_t client_recv_thread, client_send_thread;
	int length;
	int client_sock;
	int port;
	vector<string> inputLines;

	if(argc > 4) {
		printf("Too many arguments. Usage: ./c (or your output file name) <username> <port>\n");
		exit(1);
	}

	if(argc == 4) {
		string inputFile = argv[3];
		inputLines = fileToVec(inputFile);
	}
	port = atoi(argv[2]);

	strcpy(username,argv[1]);
	string u = argv[1];

	client_sock = socket(AF_INET, SOCK_STREAM, 0);
	memset(server.sin_zero,'\0',sizeof(server.sin_zero));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	char user[200];
	strcpy(user, username);
	strcat(user, ":NEW");
	printf("You are connected to the server with IP 127.0.0.1. Start chatting!\n");
	

	if(connect(client_sock,(struct sockaddr *)&server,sizeof(server)) < 0) {
		perror("Connect Failed.\n");
		exit(1);
	}
	
	pthread_mutex_lock(&mut);
	::send(client_sock, user, strlen(user), 0);
	sleep(3);
	pthread_mutex_unlock(&mut);

	inet_ntop(AF_INET, (struct sockaddr *)&server, ip, INET_ADDRSTRLEN);

	pthread_create(&client_recv_thread, NULL, receive_msg, &client_sock);

	if(argc < 4) {
		while(*(fgets(message, 500, stdin)) > 0) {
			strcpy(message2, username);
			strcat(message2 ,":");
			strcat(message2 ,message);
			length = write(client_sock, message2, strlen(message2));
			if(length < 0) 
			{
				perror("Write Failed..");
				exit(1);
			}
			memset(message,'\0',sizeof(message));
			memset(message2,'\0',sizeof(message2));
		}
	}
	else {
		for(int i = 0; i < inputLines.size(); i++) {
			pthread_mutex_lock(&mut);

			int end = inputLines.at(i).find(" ");
			string firstWord = inputLines.at(i).substr(0, end);

			if(firstWord.compare("\\SLEEP") == 0){
				string s = inputLines.at(i).substr(end + 1, inputLines.at(i).length() - end - 1);
				int sleep_amount = stoi(s);
				sleep(sleep_amount);
				pthread_mutex_unlock(&mut);
				continue;
			}
			
			string msg = u + ":" + inputLines.at(i);
			
			length = ::send(client_sock, msg.c_str(), msg.length(), 0);
			if(length < 0) 
			{
				perror("Write Failed.");
				exit(1);
			}
			pthread_mutex_unlock(&mut);
		}
	}

	pthread_join(client_recv_thread, NULL);

	close(client_sock);

}
