# CS 377 - Final Project
Members: Arushi Ahmed, Farhan Rahman, and Mukul Kudlugi

To run server (in command line):
```
g++ server.c -lpthread -o server
./server [port number]
```

To run client (in command line):
```
g++ client.c -lpthread -o client
./client [username] [port number] [optional .txt file]
```

The port number used when executing the client must be the same as the port number used when executing the server.

If you add a .txt file when executing the client, the client will run the commands in the text file and only the commands in the text file.

# Implementation
We used a struct for the client, which stored the client's username, socket number, and any rooms the client is in.
