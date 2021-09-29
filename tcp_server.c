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
#include <time.h>

#define PORT 4444
#define MAX 1048
#define MAX_CLIENTS 5
#define NAME_SIZE 32

int fd_holder=0;

pthread_mutex_t mutex;		//Create a mutex for threading

//Structures:
struct client{			//Structure for holding client information
    struct sockaddr_in address;
    int socket_fd;
    int pid;
    char name[NAME_SIZE];
};

struct queue_nodes{			//Linked list structure used to implement queue for clients
    struct queue_nodes *next;
    struct client *cli_info;
};

struct queue_nodes *head;			//Head pointer of the client queue
struct queue_nodes *tail;			//Tail pointer of the client queue

static int client_count=0;
static int pid=0;			//Process ID for clients

//Function Prototypes for the Server
void trim_str(char *str);								//Replace '\n' at the end of string with null terminator '\0'
void add_to_queue(struct client *cli);	//Add client structure to queue
void remove_from_queue(int pid);				//Remove client structure (as specified by the pid) from queue
void send_msg(char *msg, int pid);			//Send message to all clients except for the sender client
void *client_handler(void *arg);				//Thread for handling each client
void print_client_addr(struct sockaddr_in cli_addr);
void signal_handler(int sig);						//Handle ctrl-C kill signal

//Main Driver Code
int main(){
    int server_socket_fd;			//Socket ID for server
    int connect_fd;						//Socket ID for each client connected to server

    //Create a server socket using socket()
    server_socket_fd=socket(AF_INET, SOCK_STREAM, 0);
    fd_holder=server_socket_fd;
	
    //If socket creation failed
    if(server_socket_fd==-1){
    	perror("Error: ");		//Print error message
	exit(0);				//Exit
    }
    //Socket creation successful
    else{
	printf("Socket created successfully!\n");
    }

    //Define the server address & port
    struct sockaddr_in server_addr;
    memset(&server_addr, '\0', sizeof(struct sockaddr_in));

    server_addr.sin_family=AF_INET;		//Address family
    server_addr.sin_port=htons(PORT);		//Port number
    (server_addr.sin_addr).s_addr=inet_addr("127.0.0.1");		//IP address

    signal(SIGPIPE, SIG_IGN);			//Ignore pipe signals

    //Bind the server socket to our specified port and ip address
    int bind_status;
    bind_status=bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if(bind_status<0){		//If server failed to bind
	perror("Error: Server failed to bind");
	exit(1);
    }

    signal(SIGINT, signal_handler);

    //Listen for clients
    printf("Listening for connections...\n");
    int listen_status;
    listen_status=listen(server_socket_fd, 10);

    if(listen_status<0){
	perror("Error while listening: \n");
	exit(1);
    }

    pthread_t tid[MAX_CLIENTS];			//Thread IDs
    pthread_mutex_init(&mutex, NULL);			//Initialize mutex

    //Create a structure for client address
    struct sockaddr_in cli_addr;

    int i=0;
    char name_buffer[NAME_SIZE];		//Buffer for name of client

    while(1){
	socklen_t cli_length=sizeof(cli_addr);
	//Accept a connection to a client
	connect_fd=accept(server_socket_fd, (struct sockaddr *)&cli_addr, &cli_length);

	//Check if max client count is reached
	if(client_count==MAX_CLIENTS){
	    printf("Max number of clients reached! This client will be closed\n");
	    printf("The following client IP addr and port is rejected:\n");
	    print_client_addr(cli_addr);
	    printf("%d", cli_addr.sin_port);
	    close(connect_fd);
	    continue;
	}
	//Client structure configuration
	struct client *client=(struct client *)malloc(sizeof(struct client));

	client->address=cli_addr;		//Get client's socket address
	client->socket_fd=connect_fd;		//Get client's socket fd
	client->pid=pid;                //Give the client an PID
	pid++;                          //Increase the PID for next client

	//Receive client's name and store into name_buffer
	recv(client->socket_fd, name_buffer, NAME_SIZE, 0);
	//Copy name_buffer into cli->name
	strcpy(client->name, name_buffer);

	printf("%s has joined the chat\n", client->name);
            
	//Add new client connection to the queue
	add_to_queue(client);
            
	//Create a new thread for a new client that's connected
	pthread_create(&tid[i], NULL, &client_handler, (void *)client);
	i++;

	memset(name_buffer, '\0', NAME_SIZE);
	memset(&cli_addr, 0, sizeof(cli_addr));

	sleep(1);
    }

    close(server_socket_fd);		//Close server socket

    return 0;
}

//Replace the '\n' at the end of a string with a null terminator
void trim_str(char* str){
    if(str[strlen(str)-1]=='\n'){
    str[strlen(str)-1]='\0';
    }
}

//Send message to all clients except for the sender
void send_msg(char* msg, int pid){

    pthread_mutex_lock(&mutex);			//Lock the mutex

    if(head!=tail){
	struct queue_nodes *temp;
	temp=head;

	while(temp!=NULL){
	    if(temp->cli_info->pid!=pid){
		int write_status;
		write_status=send(temp->cli_info->socket_fd, msg, strlen(msg), 0);

		if(write_status<0){
		    perror("Error in sending message");
		    break;
		}
	    }

	    if(temp->next==NULL){
		break;
	    }

	    temp=temp->next;
	}
    }

    pthread_mutex_unlock(&mutex);			//Unlock the mutex
}

//One thread per client connected
void *client_handler(void *arg){
    client_count++;

    char output_buffer[MAX];				//Buffer for sending messages to clients
    char input_buffer[MAX];					//Buffer for receiving messages from clients
 
    int leave_flag=0;			//FLag to indicate when to terminate the client thread

    struct client *cli=(struct client *)arg;		//Type caste the client structure *arg (void type) passed
																								//into the thread as argument, back into type of struct client

    sprintf(output_buffer, "%s has joined the chat\n", cli->name);
    send_msg(output_buffer, cli->pid);

    memset(output_buffer, '\0', MAX);			//Clear buffer

    while(1){
	if(leave_flag==1){			//Exit while loop and terminate thread
	    break;
	}

	int receive;
	receive=recv(cli->socket_fd, input_buffer, MAX, 0);

	if(receive>0){
	    if(strlen(input_buffer)>0){
		strcpy(output_buffer, input_buffer);
		send_msg(output_buffer, cli->pid);
	    }
	}
	else if(receive==0 || strcmp(input_buffer, "exit")==0){
	    printf("%s has left the chat\n", cli->name);
	    sprintf(output_buffer, "%s has left the chat\n", cli->name);
	    send_msg(output_buffer, cli->pid);
	    leave_flag=1;
	}
	else{
	    perror("Error in receiving message: ");
	    leave_flag=1;
	}

	memset(output_buffer, '\0', MAX);
	memset(input_buffer, '\0', MAX);
    }

    remove_from_queue(cli->pid);		//Remove client from queue
    close(cli->socket_fd);					//Close the client socket
    client_count--;

    pthread_detach(pthread_self());		//Detach thread upon completion
    return NULL;
}

//Add newly connected client to the queue
void add_to_queue(struct client *cli){

    pthread_mutex_lock(&mutex);		//Lock the mutex

    if(head==NULL){		//If the queue is empty
	head=malloc(sizeof(struct queue_nodes));
	head->cli_info=cli;
	tail=head;
	tail->next=NULL;
    }
    else{			//Else, add the new client to the end of the queue
	tail->next=malloc(sizeof(struct queue_nodes));
	tail=tail->next;
	tail->cli_info=cli;
	tail->next=NULL;
    }
    pthread_mutex_unlock(&mutex);	//Unlock the mutex
}

//Remove the client (as specified by its pid) from the queue
void remove_from_queue(int pid){

    pthread_mutex_lock(&mutex);			//Lock the mutex

    if(head==tail){				//If the queue contains 1 client
	if(head->cli_info->pid==pid){
	    free(head->cli_info);
	    free(head);
	    //tail=NULL;
	}
    }
    else{								//If the queue contains more than 1 client
	struct queue_nodes* temp;

	if(head->cli_info->pid==pid){					//If client to be removed is at the head of the queue
	    temp=head;									//Set temp to head
	    head=head->next;						//Update head to the next node
	    free(temp->cli_info);				//Free the client struct pointed to by the previous head
	    free(temp);									//Free the previous head pointer
        }
        else if(tail->cli_info->pid==pid){			//If client to be removed is at the tail of the queue
	    temp=head;								//Set temp to head

	    while(temp->next!=tail){
	        temp=temp->next;			//Update temp to the second-to-last node
	    }

	tail=temp;								//Update tail to the second-to-last node
	free(temp->next->cli_info);			//Free the client struct pointed to by the last node ptr
	free(temp->next);					//Free the last node ptr
	}
	else{					//If client to be removed is anywhere in between the head & tail
	    temp=head;
	    while(temp!=NULL){
		if(temp->next->cli_info->pid==pid){
		    struct queue_nodes* delete_node;
		    delete_node=temp->next;
		    temp->next=(temp->next)->next;
		    free(delete_node->cli_info);
		    free(delete_node);
		    break;
		}
		if(temp->next==NULL){
		    break;
		}
		temp=temp->next;
	    }
        }
    }
    pthread_mutex_unlock(&mutex);				//Unlock the mutex
}

//Displays the IP address of the client that's rejected
void print_client_addr(struct sockaddr_in cli_addr){
    printf("%d.%d.%d.%d",
    (cli_addr.sin_addr.s_addr & 0xFF),
    (cli_addr.sin_addr.s_addr & 0xFF00)>>8,
    (cli_addr.sin_addr.s_addr & 0xFF0000)>>16,
    (cli_addr.sin_addr.s_addr & 0xFF000000)>>24);
}

void signal_handler(int sig){
    printf("Server exiting now...\n");

    if(head!=NULL){
	char *leave_msg="Server has closed, chatroom has ended";
	send_msg(leave_msg, -1);        //Send this message to all clients connected
    }

    //Then proceed to disconnect all clients from the client queue
    while(head!=NULL){
	if(head==tail){
	    free(head);
	    break;
	}
	else{
	    struct queue_nodes* temp=head;
	    head=head->next;
	    free(temp);
	}
    }
    exit(0);
}
