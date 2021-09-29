#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define MAX 1048
#define NAME_LENGTH 30
#define PORT 4444

volatile sig_atomic_t signal_flag=0;
int leave_flag=0;
char name[NAME_LENGTH];

//int socket_fd;		//Global variable so it can be used in threads

void *client_send_msg();    //Thread for sending msg
void *client_recv_msg();    //Thread for receiving msg
void trim_str(char* str);			//Replace the '\n' at the end of a string with a null terminator
void signal_handler(int sig);			//Handling the kill signal on terminal
void flush_stdout();
//void send_text_file(int sockfd);		//Send files to server
void send_image(int sockfd);
//void receive_text_file();
void receive_image();

int main(){
		printf("\n=========================================\n");
		printf("\n           Welcome to Chat Room\n");
		printf("\n=========================================\n");

		int socket_fd;
		printf("\nEnter your name:\n");
		//Gets the user's name
		fgets(name, NAME_LENGTH, stdin);
		trim_str(name);

		//Create a client socket using socket()
		socket_fd=socket(AF_INET, SOCK_STREAM, 0);

		//If socket creation failed
		if(socket_fd==-1){
				perror("Socket Error: ");		//Print error message
				exit(1);		//Exit
		}

		//Create an address for the client to connect to
		//For us, it will be the server's address
		struct sockaddr_in server_addr;
		memset(&server_addr, '\0', sizeof(server_addr));

		server_addr.sin_family=AF_INET;		//Address family
		server_addr.sin_port=htons(PORT);		//Port number
		server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");		//IP address: local machine

		//Connect client socket to the server address
		int connect_status;
		connect_status=connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

		if(connect_status==0){
				printf("\nSuccessfully connected to server!\n");
		}
		else{
				perror("Connection Error: ");		//Print error message
				close(socket_fd);			//Close socket
				exit(1);		//Exit
		}

		send(socket_fd, name, NAME_LENGTH, 0);	//Send name to server

		signal(SIGINT, signal_handler);			//Deals with ctr-C kill signal

		//Create a thread for sending messages
		pthread_t send_msg_thread;
		int send_msg_thread_status;
		send_msg_thread_status=pthread_create(&send_msg_thread, NULL, &client_send_msg, (void *)&socket_fd);
		if(send_msg_thread_status!=0){
				perror("Error while creating thread: ");
				exit(1);
		}

		//Create a thread for receiving messages
		pthread_t recv_msg_thread;
		int recv_msg_thread_status;
		recv_msg_thread_status=pthread_create(&recv_msg_thread, NULL, &client_recv_msg, (void *)&socket_fd);
		if(recv_msg_thread_status!=0){
				perror("Error while creating thread: ");
				exit(1);
		}

		while(1){
				//Exit if either flag is set
				if(signal_flag!=0 || leave_flag!=0){
						break;
				}
		}

		close(socket_fd);		//Close socket

		return 0;
}

void signal_handler(int sig){
		signal_flag=1;
		printf("Exiting chat room...\n");
}

void flush_stdout(){
		printf("%s",">");
		fflush(stdout);
}

//Replace the '\n' at the end of a string with a null terminator '\0'
void trim_str(char* str){
		if(str[strlen(str)-1]=='\n'){
				str[strlen(str)-1]='\0';
		}
}

//Thread for sending messages
void *client_send_msg(void *arg){
		int socket_fd=*((int *)arg);
		char message_buffer[MAX]={};
		char send_buffer[NAME_LENGTH+MAX];
		char file_name[MAX];

		while(1){
				flush_stdout();
				fgets(message_buffer, MAX, stdin);
				trim_str(message_buffer);


				if(strcmp(message_buffer, "exit")==0){
						sprintf(send_buffer, "%s has left the chat\n", name);
						//Notify server that the client has left the chat
						send(socket_fd, send_buffer, (NAME_LENGTH+MAX), 0);
						printf("Exiting chatroom...\n");
						leave_flag=1;
						break;
						}
				else{
						//Message will be sent in the following format:
						//Name: *message
						sprintf(send_buffer, "%s: %s", name, message_buffer);
						send(socket_fd, send_buffer, strlen(send_buffer), 0);
				}

				//Clear the buffers
				memset(message_buffer, '\0', MAX);
				memset(send_buffer, '\0', (NAME_LENGTH+MAX));
		}
}

//Thread for receiving messages
void *client_recv_msg(void *arg){
		int socket_fd=*((int *)arg);
		char recv_msg_buffer[NAME_LENGTH+MAX];

		while(1){
				int recv_status;
				recv_status=recv(socket_fd, recv_msg_buffer, (NAME_LENGTH+MAX), 0);

				if(recv_status>0){
						printf("%s\n", recv_msg_buffer);
						flush_stdout();
				}
				else if(recv_status==0){
						break;
				}
				else{
						perror("Error in receiving message: ");
						break;
				}
				//Clear buffer
				memset(recv_msg_buffer, '\0', (NAME_LENGTH+MAX));
		}
}
