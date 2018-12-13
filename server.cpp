#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <vector>
#include <map>
#include <set>

using namespace std;

//Struct that contains information about the client
struct user 
{
	string username; //Client nickname
	int socket; //Client socket
	string curr_room; //Current location of the room
	char ip[INET_ADDRSTRLEN]; // Client IP Address
};

//Hashmaps to store the list of clients and the list of rooms
map<string, user> clients_list;
map<string, set<string> > rooms_list;

int clients[100];
int num_clients = 0;
int num_rooms = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Function to grab username from message
string get_username(string message){
    int len = message.find(":");
    return message.substr(0, len);
}

//Function to send direct message to another user on the system
void send_dm(string message, int sockno) {
    pthread_mutex_lock(&mutex);
    
	int start = message.find(":") + 1;
	if(message.at(start) != '\\'){
		int end = message.find(" ", start);
		string msg = message.substr(start, (end - start));
		//if(msg.at(msg.length() - 1) == '\n') msg = msg.substr(0, msg.length() - 1);
		msg += " command not recognized.\nType \\HELP for available commands."; 
        if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
            perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
		return;
	}

    int next = message.find(" ", start);
    string message_to_send = message.substr(next + 1, (message.length() - next - 1));
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

	if(username.compare(other_username) == 0){
		string msg = "You cannot send a message to yourself."; 
        if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
            perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
		return;
	}

	for(map<string, set<string> >::iterator mit = rooms_list.begin(); mit != rooms_list.end(); mit++){
        if(other_username.compare(mit->first) == 0){
			other_sockno = -2;
			set<string> s = rooms_list[other_username];
			message_to_send = username + ": " + message_to_send;

			for(set<string>::iterator it = s.begin(); it != s.end(); it++){
				int socket = clients_list[*it].socket;
				if(::send(socket, message_to_send.c_str(), message.length(), 0) < 0) {
					perror("Failed To Send");
				}
			}
		}
	}

    if(other_sockno == -1){
        string msg = other_username + " command not recognized.\nType \\HELP for available commands."; 
        if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
            perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
    }

    else if(other_sockno != -2){
        message_to_send = username + ": " + message_to_send;

        if(::send(sockno, message_to_send.c_str(), message.length(), 0) < 0) {
            perror("Failed To Send");
        }
        if(::send(other_sockno, message_to_send.c_str(), message.length(), 0) < 0) {
            perror("Failed To Send");
        }
    }

    pthread_mutex_unlock(&mutex);

}

//Function when \JOIN is called, joins requested room
void join(int sockno, string message) {
	pthread_mutex_lock(&mutex);

	int start = message.find(":") + 1;
    start = message.find(" ", start) + 1;
    int end = message.find(" ", start);
    string username = message.substr(start, (end - start));
	
	start = message.find(" ", end) + 1;
    end = message.find(" ", start);
	string roomname = message.substr(start, (message.length() - start - 1));
	
	if(rooms_list.find(roomname) == rooms_list.end()){
		set<string> s;
		s.insert(username);
		rooms_list[roomname] = s;
		clients_list[username].curr_room = roomname;
		num_rooms++;
	}

	else{
		rooms_list[roomname].insert(username);
		clients_list[username].curr_room = roomname;
	}

	set<string> s = rooms_list[roomname];
	string msg = username + " just joined the room " + roomname + ".";

	for(set<string>::iterator it = s.begin(); it != s.end(); it++){
		int socket = clients_list[*it].socket;
		if(::send(socket, msg.c_str(), msg.length(), 0) < 0) {
			perror("Failed To Send");
		}
	}

	pthread_mutex_unlock(&mutex);
}

//Function when \ROOMS is called, prints list of existing rooms
void print_rooms(int socket) {
	pthread_mutex_lock(&mutex);
	if(rooms_list.size() == 0){
		string msg = "There is no room available.";
		if(::send(socket, msg.c_str(), msg.length(), 0) < 0) {
			perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
		return;
	}

	string msg = "Rooms: ";
	int i = 0;

    for(map<string, set<string> >::iterator it = rooms_list.begin(); it != rooms_list.end(); it++){
        msg += it->first;
        if(i++ != rooms_list.size() - 1) msg += ", ";
	}
	
	if(::send(socket, msg.c_str(), msg.length(), 0) < 0) {
		perror("Failed To Send");
	}
	pthread_mutex_unlock(&mutex);
}

//Function when \LEAVE is called, leaves current room.
void leave(string message, int sockno) {
	pthread_mutex_lock(&mutex);
	string username = get_username(message);

	if(clients_list[username].curr_room.compare("-") == 0){
		string msg = "You are not in any room.";
		if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
			perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
		return;
	}

	string roomname = clients_list[username].curr_room;
	rooms_list[roomname].erase(username);
	clients_list[username].curr_room = "-";

	set<string> s = rooms_list[roomname];
	string msg = username + " just left the room " + roomname + ".";

	for(set<string>::iterator it = s.begin(); it != s.end(); it++){
		int socket = clients_list[*it].socket;
		if(::send(socket, msg.c_str(), msg.length(), 0) < 0) {
			perror("Failed To Send");
		}
	}

	msg = "GOODBYE";
	if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
		perror("Failed To Send");
	}

	pthread_mutex_unlock(&mutex);
}

//Function when \WHO is called, prints list of users in current room.
void who(string message, int sockno) {
	pthread_mutex_lock(&mutex);

	string username = get_username(message);
	if(clients_list[username].curr_room.compare("-") == 0){
		string msg = "You are not in any room.";
		if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
			perror("Failed To Send");
		}
		pthread_mutex_unlock(&mutex);
		return;
	}
	
	string msg = "People in this room: ";
	string roomname = clients_list[username].curr_room;
	
	set<string> s = rooms_list[roomname];
	int i = 0;

	for(set<string>::iterator it = s.begin(); it != s.end(); it++){
		msg += *it;
		if(i++ != s.size() - 1) msg += ", ";
	}

	if(::send(sockno, msg.c_str(), msg.length(), 0) < 0) {
		perror("Failed To Send");
	}
    
    pthread_mutex_unlock(&mutex);
}

//Function when \HELP is called, prints out all allowed commands
void help(int socket) {
	string msg = "\nHelp Information:\n\n\\JOIN: To join a room, type \\JOIN <your username> <roomname>\n\\ROOMS: available rooms\n\\LEAVE: leaving a room\n\\WHO: which people are in the room\n\\HELP: list of all the commands\n\\nickname message: To send a message to any user or a room\n(even if you are not part of it), type '\\<other person/room's username/roomname> <message>'\n";
	pthread_mutex_lock(&mutex);
	if(send(socket, msg.c_str(), msg.length(), 0) < 0) {
		perror("Failed To Send");
	}
	pthread_mutex_unlock(&mutex);
}

void register_user(string message, int sockno){
	string username = get_username(message);

	user *u = new user();
	u->socket = sockno;
	u->username = username;
	u->curr_room = "-";

    clients_list[username] = *u;
}

//Takes in each command or message a user is trying to send. 
//When the user calls a command that is implemented, it will call the function for the command to work.
void check_msg(string message, int sockno) {
	int start = message.find(":") + 1;
	int end = message.find(" ", start);
	string command = message.substr(start, (end - start));
	

	if(command.compare("\\JOIN") == 0) {
		join(sockno, message);
	}
	else if(command.compare("\\ROOMS") == 0) {
		print_rooms(sockno);
	}
	else if(command.compare("\\LEAVE") == 0) {
		leave(message, sockno);
	}
	else if(command.compare("\\WHO") == 0) {
		who(message, sockno);
	}
	else if(command.compare("\\HELP") == 0) {
		help(sockno);
	}
	else if(command.compare("NEW") == 0) {
		register_user(message, sockno);
	}
	else {
		send_dm(message, sockno);
	}
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
	printf("Client with IP %s Just Disconnected\n", client.ip);
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


int main(int argc,char *argv[])
{
	//Checking if the client writes too many or not enought argunments. If either one it will give directions on what to write.
	if(argc > 2) {
		perror("Too many arguments. Usage: ./s (or your output file name) <port>\n");
		exit(1);
	}

	if(argc == 1) {
		perror("Include port number as argument. Usage: ./s (or your output file name) <port>\n");
		exit(1);
	}

	//Client and server ID address information
	struct sockaddr_in server;
	struct sockaddr_in client;

	int server_sock;
	int client_sock;
	int port;
	int optval = 1;

	//Waiting for incoming connections and creates a new thread to handle the connections
	socklen_t client_size;
	pthread_t client_thread;

	//For the client and the address
	struct user cl;
    char ip[INET_ADDRSTRLEN]; 
	
	port = atoi(argv[1]);
	server_sock = socket(AF_INET,SOCK_STREAM,0);

	/* Eliminates "Address already in use" error from bind */
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int)) < 0){
		return -1;
	}

	//Listening will be an endpoint for all requests to port on any IP address for this host.
	memset(server.sin_zero,'\0',sizeof(server.sin_zero));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_size = sizeof(client);
    
	//Binding the address
	if(::bind(server_sock,(struct sockaddr *)&server,sizeof(server)) >= 0) {
		printf("Binding Successful\n");
	}
	else {
		perror("Binding Unsuccessful\n");
		exit(1);
	}

	//Listing for the socket and ready to accept connection request
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
		printf("Client with IP %s Connected\n", ip);
		cl.socket = client_sock;
		strcpy(cl.ip, ip);
        clients[num_clients++] = client_sock;
		pthread_create(&client_thread, NULL, receive_msg, &cl);
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}
