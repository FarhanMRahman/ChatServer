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

# Server Implementation
We created a struct for the client, which stores the client's username, socket number, ip address, and any rooms the client is in.

We store the list of clients in a hashmap using the client's username as the key, and the client struct itself as the value.

We store the list of rooms in a hashmap using the room's name as the key, and a set containing the usernames of every client currently in that room as the value.

When a client connects to our server, we create a new thread for that client, and send the message received to our check_msg() method, which checks the received message for any of the required commands, like \JOIN or \HELP. The method calls a function based on what the message contains, and we have an individual method for each possible command.

Our creative feature was the ability for a client to send a message to a room, even if the client isn't currently in said room. Our server recognizes if a message is directed to a specific room and send the message to every member of the room.

# Client Implementation
We created our client file with the ability to take an .txt file and use commands inside the file. If the .txt file is added when running the client, the user will lose the ability to use manual commands for that session. To regain the ability to use manual commands, rerun the client without the .txt file.
