#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct user {
  int socket;
  char ip[INET_ADDRSTRLEN];
  char username[100];
  char curr_room[100];
};

struct user client_list[500];
int most_recent_client = 0;

struct room {
  struct user users[100];
  char room_name[100];
};

struct room room_list[500];

//Sliding in the DMs
void send_dm(char *message, struct user client) {

}

void join(struct user client, char *message) {

}

void print_rooms(struct user client) {
  if(send(client.socket, "yo", 2, 0) < 0) {
    perror("Couldn't send message.");
  }
}

void leave(struct user client) {

}

void who(struct user client) {

}

void help(struct user client) {

}

void check_msg(char *message, struct user client) {
  char* command = strtok(message, " ");

  if(strcmp(command, "\\JOIN") == 0) {
    join(client, message);
  }
  else if(strcmp(command, "\\ROOMS") == 0) {
    print_rooms(client);
  }
  else if(strcmp(command, "\\LEAVE") == 0) {
    leave(client);
  }
  else if(strcmp(command, "\\WHO") == 0) {
    who(client);
  }
  else if(strcmp(command, "\\HELP") == 0) {
    help(client);
  }
  else {
    send_dm(message, client);
  }

}

void *receive(void *u) {
  struct user client = *(struct user *) u;
  char message[100];

  while(1) {
    int len = recv(client.socket, message, 100, 0);
    if(len > 0) {
      message[len] = '\0';
      check_msg(message, client);
      memset(message, '\0', sizeof(message));
    }
    else {
      break;
    }
  }
}

int main(int argc, char *argv[]) {

  if(argc > 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  if(argc == 1) {
    printf("Include port number as argument\n");
    exit(0);
  }

  struct sockaddr_in server;
  struct sockaddr_in client_addr;

  socklen_t client_size;
  pthread_t client_thread;

  int server_sock, client_sock, port;

  port = atoi(argv[1]);
	server_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(server.sin_zero,'\0',sizeof(server.sin_zero));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("172.30.124.57");
  struct user client;
  client_size = sizeof(client_addr);
  char ip[INET_ADDRSTRLEN];

  if(bind(server_sock, (struct sockaddr *) &server, sizeof(server) == 0)) {
    printf("Binding successful\n");
  }
  else {
    perror("Binding unsuccessful\n");
    exit(1);
  }

  if(listen(server_sock, 10) == 0) {
    printf("Listening successful\n");
  }
  else {
    perror("Listening unsuccessful\n");
    exit(1);
  }

  while(1) {
    client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &client_size);
    if(client_sock == 0) {
      printf("Accept successful\n");
    }
    else {
      perror("Accept unsuccessful\n");
      exit(1);
    }

    pthread_mutex_lock(&mutex);
    inet_ntop(AF_INET, (struct sockaddr *) &client_addr, ip, INET_ADDRSTRLEN);
    printf("Connected to %s\n", ip);

    client.socket = client_sock;
    strcpy(client.ip, ip);

    client_list[most_recent_client++] = client;

    pthread_create(&client_thread, NULL, receive, &client);
    pthread_mutex_unlock(&mutex);
  }
  return 0;
}