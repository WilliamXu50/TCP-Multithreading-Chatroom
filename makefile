compile:
	gcc tcp_server.c -lpthread -o server
	gcc tcp_client.c -lpthread -o client
