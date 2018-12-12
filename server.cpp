#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <map>
#include <set>

using namespace std;

struct user {
	string username;
	int socket; 
	char ip[INET_ADDRSTRLEN];
	string curr_room;
};

map<string, user> clients_list;
map<string, set<string>> rooms_list;

int clients[100];

int num_clients = 0;
int num_rooms = 0;



pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

string get_username(string message){
    int len = message.find(":");
    return message.substr(0, len);
}

void send_dm(string message, int sockno) {
    pthread_mutex_lock(&mutex);
    
    int start = message.find(":") + 1;
    int next = message.find(" ", start);
    string message_to_send = message.substr(next + 1, (message.length() - next - 2));
    string other_username;
    if(next != -1) other_username = message.substr(start + 1, (next - start - 1));
    else{
        other_username = message.substr(start + 1);
        other_username = other_username.substr(0, other_username.length() - 1);
    }

    int other_sockno = -1;
    string username;
    for(map<string, user>::iterator it = clients_list.begin(); it != clients_list.end(); it++){
        if(other_username.compare(it->first) == 0){
            other_sockno = it->second.socket;
        }
        if(sockno == it->second.socket){
            username = it->first;
        }
    }

    if(other_sockno == -1){
        string msg = other_username + " command not recognized."; 
        if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
            perror("sending failure");
        }
    }

    else{
        message_to_send = username + ": " + message_to_send;

        printf("\nmessage: %s\n", message_to_send.c_str());
        printf("username: %s\n", username.c_str());
        printf("sockno: %d\n", sockno);
        printf("other_username: %s\n", other_username.c_str());
        printf("other_sockno: %d\n", other_sockno);

        if(::send(sockno, message_to_send.c_str(), message.length(), 0) < 0) {
            perror("sending failure");
        }
        if(::send(other_sockno, message_to_send.c_str(), message.length(), 0) < 0) {
            perror("sending failure");
        }
    }

    pthread_mutex_unlock(&mutex);

}

void join(int sockno, string message) {
	pthread_mutex_lock(&mutex);
	
	int start = message.find(":") + 1;
    start = message.find(" ", start) + 1;
    int end = message.find(" ", start);
    string username = message.substr(start, (end - start));
	
	printf("user: %s\n", username.c_str());

	start = message.find(" ", end) + 1;
    end = message.find(" ", start);
	string roomname = message.substr(start, (message.length() - start - 1));
	
	printf("room: %s\n", roomname.c_str());

	if(rooms_list.find(roomname) == rooms_list.end()){
		set<string> s;
		s.insert(username);
		rooms_list[roomname] = s;
		clients_list[username].curr_room = roomname;
		num_rooms++;

		string msg = username + " joined room " + roomname + ".";
		if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
			perror("sending failure");
		}
	}

	else{
		rooms_list[roomname].insert(username);
		clients_list[username].curr_room = roomname;
		string msg = username + " joined room " + roomname + ".\n";
		if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
			perror("sending failure");
		}
	}

	pthread_mutex_unlock(&mutex);
}

// When \ROOM is being called
void print_rooms(int socket) {
    pthread_mutex_lock(&mutex);
	string msg = "Rooms: ";
	int i = 0;

    for(map<string, set<string>>::iterator it = rooms_list.begin(); it != rooms_list.end(); it++){
        msg += it->first;
        if(i++ != num_rooms - 1) msg += ", ";
    }
	
	if(::send(socket, msg.c_str(), msg.length(), 0) < 0) {
		perror("sending failure");
	}
	pthread_mutex_unlock(&mutex);
}

void leave(string message, int sockno) {
	pthread_mutex_lock(&mutex);
	
	string username = get_username(message);
	string roomname = clients_list[username].curr_room;
	rooms_list[roomname].erase(username);

	string msg = username + " left this room.\n";
	if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
		perror("sending failure");
	}

	pthread_mutex_unlock(&mutex);
}

void who(string message, int sockno) {
	pthread_mutex_lock(&mutex);
	
	string msg = "People in this room: ";
	string username = get_username(message);
	string roomname = clients_list[username].curr_room;
	printf("u: %s, r: %s\n", username.c_str(), roomname.c_str());
	
	set<string> s = rooms_list[roomname];
	int i = 0;

	for(set<string>::iterator it = s.begin(); it != s.end(); it++){
		msg += *it;
		if(i != s.size() - 1) msg += ", ";
	}

	if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
		perror("sending failure");
	}
    
    pthread_mutex_unlock(&mutex);
}

// When \HELP is being called. It will send a message of all the commands with details
void help(int socket) {
    string msg = "help info\n";
	pthread_mutex_lock(&mutex);
	if(send(socket, msg.c_str(), msg.length(), 0) < 0) {
		perror("sending failure");
	}
	pthread_mutex_unlock(&mutex);
}

void register_user(string message, int sockno){
	string username = get_username(message);

	user *u = new user();
	u->socket = sockno;
	u->username = username;

    clients_list[username] = *u;
    //printf("%s: %d\n", u->username.c_str(), clients_list[username].socket);
}

//Takes in each command or message a user is trying to send. 
//When the user calls a command that is implemented, it will call the function for the command to work, else it will give them an error. 
void check_msg(char *message, int sockno) {
  char msgArray[500];
  strcpy(msgArray, message);
  char command[100];
  for(int i = 0; i < strlen(message); i++) {
	  if(msgArray[i] == ':') {
		  int j = i + 1;
		  int k = 0;
		  while(msgArray[j] != ' ' && msgArray[j] != '\0') {
			  command[k] = msgArray[j];
			  j++;
			  k++;
		  }
		  break;
	  }
  }

  if(strcmp(command, "\\JOIN") == 0) {
    join(sockno, message);
  }
  else if(strcmp(command, "\\ROOMS\n") == 0) {
    print_rooms(sockno);
  }
  else if(strcmp(command, "\\LEAVE\n") == 0) {
    leave(message, sockno);
  }
  else if(strcmp(command, "\\WHO\n") == 0) {
    who(message, sockno);
  }
  else if(strcmp(command, "\\HELP\n") == 0) {
    help(sockno);
  }
  else if(strcmp(command, "NEW") == 0) {
    register_user(message, sockno);
  }
  else {
    send_dm(message, sockno);
  }

}

void sendtoall(char *msg,int curr)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < num_clients; i++) {
		if(clients[i] != curr) {
			if(send(clients[i],msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}

//The server will take in the message the client is trying to send to a private or group message
void *receive_msg(void *sock)
{
	struct user client = *((struct user *)sock);
	char message[500];
	int len;

	while((len = recv(client.socket, message, 500, 0)) > 0) {
        message[len] = '\0';
        check_msg(message, client.socket);
        memset(message, '\0', sizeof(message));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",client.ip);
	for(int i = 0; i < num_clients; i++) {
		if(clients[i] == client.socket) {
			int j = i;
			while(j < num_clients-1) {
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	num_clients--;
	pthread_mutex_unlock(&mutex);
}

// The setup of the server. Binds and listening for socket and make sure it accepts the connection request. 
// Also checks that the client is connected to the right server.
int main(int argc,char *argv[])
{
	if(argc > 2) {
		perror("Too many arguments");
		exit(1);
	}

	if(argc == 1) {
		perror("Include port number as argument");
		exit(1);
	}

	struct sockaddr_in server;
	struct sockaddr_in client;

	int server_sock;
	int client_sock;
	int port;

	socklen_t client_size;
	pthread_t client_thread;

	struct user cl;
    char ip[INET_ADDRSTRLEN];
	
	/*
    // dummy data
    num_rooms = 3;
    rooms_list.push_back("Monkey Bar");
    rooms_list.push_back("Spoke");
    rooms_list.push_back("Lit");  
    

	//
	*/

	port = atoi(argv[1]);
	server_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(server.sin_zero,'\0',sizeof(server.sin_zero));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_size = sizeof(client);
    
	if(::bind(server_sock,(struct sockaddr *)&server,sizeof(server)) >= 0) {
		printf("Binding Successful\n");
	}
	else {
		perror("Binding Unsuccessful\n");
		exit(1);
	}

	if(listen(server_sock, 1024) >= 0) {
		printf("Listening Successful\n");
	}
	else {
		perror("Listening Unsuccessful\n");
		exit(1);
	}

	while(1) {
		if((client_sock = accept(server_sock, (struct sockaddr *) &client, &client_size)) < 0) {
			perror("Accept Unsuccessful");
			exit(1);
        }
        
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *) &client, ip, INET_ADDRSTRLEN);
		printf("%s Connected\n", ip);
		cl.socket = client_sock;
		strcpy(cl.ip, ip);
        clients[num_clients++] = client_sock;
		pthread_create(&client_thread, NULL, receive_msg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}