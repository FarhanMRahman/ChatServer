#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct user {
  int socket;
  char ip[INET_ADDRSTRLEN];
  char username[100];
};

struct room {
  struct user users[100];
  char room_name[100];
};

int main(int argc, char *argv[]) {
  struct sockaddr_in server;
  struct sockaddr_in client_addr;

  int server_sock, client_sock, port;

  char message[100];
}