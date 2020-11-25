#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>

#define MESSAGE_BUFF 500
#define USERNAME_BUFF 10

char decision[512];

typedef struct {
	char* prompt;
	int socket;
}thread_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//與server連線
void* connect_to_server(int socket_fd, struct sockaddr_in *address)
{
	int response = connect(socket_fd, (struct sockaddr *) address, sizeof *address);
	if (response < 0)
    {
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		exit(1);
	}
	else
    {
		printf("Connected\n");
	}
}

//打字、傳送訊息
void* send_message(char prompt[USERNAME_BUFF+4], int socket_fd, struct sockaddr_in *address, char username[])
{
	char message[MESSAGE_BUFF];
	char buff[MESSAGE_BUFF];
	char notice[]="/send";
	memset(message, '\0', sizeof(message));
	memset(buff, '\0', sizeof(buff));

	send(socket_fd, username, strlen(username), 0);
	
	while(fgets(message, MESSAGE_BUFF, stdin)!=NULL)
    {
		printf("%s",prompt);
		if(strncmp(message, "/quit", 5)==0)
        {
			printf("Close connection...\n");
			send(socket_fd, message, strlen(message), 0);
			exit(0);
		}
		else if(strncmp(message, "/send", 5) == 0)
        {
			send(socket_fd,message,sizeof(message),0);
		}
		else
        {
			send(socket_fd, message, strlen(message), 0);
			memset(message, '\0', sizeof(message));
		}
  	}
}

void* receive(void* threadData)
{
	int socket_fd, response;
	char message[MESSAGE_BUFF];
	thread_data* pData = (thread_data*)threadData;
	socket_fd = pData->socket;
	char* prompt = pData->prompt;

	char buff[MESSAGE_BUFF];
	char buff2[2];

	char confirm[]="<SERVER> Confirm?\n";
	char game[]="<Server> Starting the game...\n";
	char state[]="<Server> waiting...\n";
	char turn[]="<Server> it's your turn\n";
	char error1[]="<Server> Error,this place is chosen.\n";
    char error2[]="<Server> Error,out of range.\n";
	char result1[]="<Server> You win the game!\n";
	char result2[]="<Server> You lose the game!\n";
	char ok[]="ok";
	
//接收訊息
	while(1)
    {
		memset(message, '\0', sizeof(message));
		memset(buff, '\0', sizeof(buff));
		memset(decision, '\0', sizeof(decision));
		memset(confirm,'\0',sizeof(confirm));
		memset(state,'\0',sizeof(state));
		memset(turn,'\0',sizeof(turn));
		memset(error1,'\0',sizeof(error1));
		memset(error2,'\0',sizeof(error2));
		memset(result1,'\0',sizeof(result1));
	    memset(result2,'\0',sizeof(result2));
		fflush(stdout);
		response = recv(socket_fd, message, MESSAGE_BUFF, 0);
		if (response == -1)
        {
			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			break;
		}
		if (response == 0)
        {
			printf("\nPeer disconnected\n");
			break;
		}
//遊戲邀請處理
		if(strcmp(message,"<SERVER> Confirm?\n")==0)
        {
			printf("waiting for acception.\n");
		}
		else if(strcmp(message,turn)==0)
        {
		    scanf("%s",decision);
			send(socket_fd, decision, strlen(decision), 0);
		}
		else if(strcmp(message,error1)==0)
        {
			printf("%s",error1);
			printf("Please choose another place~\n");
		}
		else if(strcmp(message,error2)==0)
        {
		    printf("%s",error2);
			printf("Please choose another place~\n");
		}
		else if(strcmp(message,result1)==0)
        {
			printf("%s",result1);
			fflush(stdout);
		}
		else if(strcmp(message,result2)==0)
        {
			printf("%s",result2);
			fflush(stdout);
		}
		else if(strcmp(message, state)==0)
        {
			printf("%s",state);
			fflush(stdout);
		}
		else
        {
			printf("%s", message);
			printf("%s", prompt);
			fflush(stdout);
		}
	}
}

int main(int argc, char**argv)
{
	long port = strtol(argv[2], NULL, 10);
	struct sockaddr_in address, cl_addr;
	char * server_address;
	int socket_fd, response;
	char prompt[USERNAME_BUFF+4];
	char username[USERNAME_BUFF];
	pthread_t thread;

//檢查arguments
	if(argc < 3)
    {
		printf("Usage: ./cc_thread [IP] [PORT]\n");
		exit(1);
	}

//進入大廳
	printf("Enter your name: ");
	fgets(username, USERNAME_BUFF, stdin);
	username[strlen(username) - 1] = 0;
	strcpy(prompt, username);
	strcat(prompt, "> ");
//連線設定
	server_address = argv[1];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_address);
	address.sin_port = htons(port);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	connect_to_server(socket_fd, &address);

//建thread data
	thread_data data;
	data.prompt = prompt;
	data.socket = socket_fd;

//建thread接收訊息
	pthread_create(&thread, NULL, (void *)receive, (void *)&data);

//傳送訊息
	send_message(prompt, socket_fd, &address,username);

//關閉
	close(socket_fd);
	pthread_exit(NULL);
	return 0;
}