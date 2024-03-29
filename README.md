# TCP-Multithreading-Chatroom

This is a TCP based chatroom written in C, in Ubuntu. This program is the completion of the final project from my real time operating system
course (last chapter regarding networking in Linux). The program has 2 source files: tcp_server.c & tcp_client.c

# Basic Design:
One terminal will compile and run `tcp_server.c`. This terminal will act as the server in the TCP network.
The user can open multiple terminals and run the `tcp_client.c` executables. These terminals will act as the clients of the TCP network.
The maximum number of clients (terminals running `tcp_client.c`) is defined in the source code. If this count is exceeded, the new client 
will be rejected. 

Now the TCP chatroom is setup. The server will handle accepting all connections as well as redirecting messages from one client to all others.
One client terminal can send text messages to all other client terminals, and so on. If the server terminal is 
closed (either by literally closing the terminal or killing the process), the TCP network will be broken.

# How to Run:
- In the project directory, run `make`. 
- This will compile `tcp_server.c` and `tcp_client.c` and create executables `server` and `client` respectively
- Run `./server` to create a server (Very Important: create a server terminal first, otherwise the clients will have nothing to connect to!)
- Then open up new terminals, navigate to the project directory and run `./client` for each (right now, maximum of 5 clients supported)
- Done! Now the terminals can communicate with each other in a public chatroom style.
