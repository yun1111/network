#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>

#define MAXLINE 512
#define MAXMEM 10
#define NAMELEN 20
#define SERV_PORT 8080
#define LISTENQ 5

int num_game=0;
int listenfd,connfd[MAXMEM];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char user[MAXMEM][NAMELEN];
char password[100][100];
void Quit();
void rcv_snd(int n);
void Game(int p1,int p2,int num);
void displayboard(int num);
int move(int palyer,char op,char ret,int num);
int row=3,col=3;
char iswin(char array[10][row][col],int num);
static int IsFull(char arr[10][row][row],int num);
char arr[3][3];
char array[10][3][3];



//關閉server
void Quit()
{
	char msg[10];
	while(1)
    {
		scanf("%s", msg);
		if(strcmp("/quit",msg)==0)
        {
			printf("Bye Bye!\n");
			close(listenfd);
			exit(0);
		}
	}
}
 
void rcv_snd(int n)
{
	char msg_notify[MAXLINE];
	char msg_recv[MAXLINE];
	char msg_send[MAXLINE];
	char who[MAXLINE];
	char name[NAMELEN];
	char message[MAXLINE];

	char msg1[]="<SERVER> Who do you want to send? ";
	char msg2[]="<SERVER> Complete.\n";
	char msg3[]="<SERVER> Rrefuse to accept.\n";
	char msg4[]="<SERVER> Download...\n";
	char msg5[]="<SERVER> Confirm?\n";
	char msg6[]="ok";
	char check[MAXLINE];
	char ok[3];

	int i=0;
	int retval;

//獲得client的名字
    
	int length;
	length = recv(connfd[n], name, NAMELEN, 0);
	if(length>0)
    {
		name[length] = 0;
		strcpy(user[n], name);
	}
//告知所有人有新client加入
	memset(msg_notify, '\0', sizeof(msg_notify));
	strcpy(msg_notify, name);
	strcat(msg_notify, " join\n");
	for(i=0; i<MAXMEM; i++)
    {
		if(connfd[i]!=-1)
        {
			send(connfd[i], msg_notify, strlen(msg_notify), 0);
		}
	}
//接收某client的訊息並轉發
	while(1)
    {
		memset(msg_recv, '\0', sizeof(msg_recv));
		memset(msg_send, '\0', sizeof(msg_send));
		memset(message,'\0',sizeof(message));
		memset(check,'\0',sizeof(check));
		
		if((length=recv(connfd[n], msg_recv, MAXLINE, 0))>0)
        {
			msg_recv[length]=0;
//輸入quit離開
			if(strncmp("/quit", msg_recv,5)==0)
            {
				close(connfd[n]);
				connfd[n]=-1;
				pthread_exit(&retval);
			}
//輸入chat傳給特定人
			else if(strncmp("/chat", msg_recv, 5)==0)
            {
				printf("private message...\n");
				send(connfd[n], msg1, strlen(msg1), 0);
				length = recv(connfd[n], who, MAXLINE, 0);
				who[length]=0;
				strcpy(msg_send, who);
				strcat(msg_send, ">");
				msg_send[strlen(who)-1]='>';
				send(connfd[n], msg_send, strlen(msg_send), 0);
				length = recv(connfd[n], message, MAXLINE, 0);
				message[length]=0;

		        strcpy(msg_send,"private message from ");
				strcat(msg_send, name);
				strcat(msg_send, ": ");
				strcat(msg_send, message);

				for(i=0; i<MAXMEM; i++)
                {
					if(connfd[i]!=-1)
                    {
						if(strncmp(who, user[i], strlen(who)-1)==0)
                        {
							send(connfd[i], msg_send, strlen(msg_send), 0);
						}
					}
				}
			}
//傳送遊戲邀請
			else if(strncmp("/send", msg_recv, 5)==0)
            {
				pthread_mutex_lock(&mutex);
				send(connfd[n], msg1, strlen(msg1), 0);
				pthread_mutex_unlock(&mutex);
				length = recv(connfd[n], who, MAXLINE,0);
				who[length]=0;
				printf("send to %s\n", who);
//server傳送確認
                pthread_mutex_lock(&mutex);
				send(connfd[n], msg5, strlen(msg5), 0);
				printf("confirm\n");
				pthread_mutex_unlock(&mutex);
				printf("receive\n");
//詢問是否要接收
				strcpy(msg_send, "<SERVER> ");
				strcat(msg_send, name);
				strcat(msg_send, " invite you to play ox game. Receive? (y/n)\n");

				for(i=0; i<MAXMEM; i++)
                {
					if(connfd[i]!=-1)
                    {
						if(strncmp(who, user[i], strlen(who)-1)==0)
                        {
							pthread_mutex_lock(&mutex);
							send(connfd[i], msg_send, strlen(msg_send), 0);
							pthread_mutex_unlock(&mutex);

							length=0;
							memset(check,'\0',sizeof(check));
							length = recv(connfd[i], check, MAXLINE, 0);
							check[length]=0;

							printf("Ans=");
//Yes開始遊戲
							if(strncmp(check, "Y", 1)==0 || strncmp(check, "y", 1)==0)
                            {
								printf("yes\n");
								send(connfd[n], "Accept!\n", strlen("Accept!\n"),0);
								printf("Starting the game...\n");
								send(connfd[n], "<Server> Starting the game...\n", strlen("<Server> Starting the game...\n"),0);
								send(connfd[i], "<Server> Starting the game...\n", strlen("<Server> Starting the game...\n"),0);
								num_game++;
								if(num_game==1)
                                {
								  Game(n,i,num_game);
								}
								else if(num_game>1)
                                {
								  Game(n,i,num_game);
								}
							}
//No取消邀請
							else if(strncmp(check, "N", 1)==0 || strncmp(check, "n", 1)==0)
                            {
								printf("no\n");
								send(connfd[n], msg3, strlen(msg3), 0);
								memset(message, '\0', sizeof(message));
							}
						}
					}
				}
			}
//顯示目前在線
			else if(strncmp("/list", msg_recv, 5)==0)
            {
				strcpy(msg_send, "<SERVER> Online:");
				for(i=0; i<MAXMEM; i++)
                {
					if(connfd[i]!=-1)
                    {
						strcat(msg_send, user[i]);
						strcat(msg_send, " ");
					}
				}
				strcat(msg_send, "\n");
				send(connfd[n], msg_send, strlen(msg_send), 0);
			}
//直接傳給每個人
			else
            {
				strcpy(msg_send, name);
				strcat(msg_send,": ");
				strcat(msg_send, msg_recv);

				for(i=0;i<MAXMEM;i++)
                {
					if(connfd[i]!=-1)
                    {
						if(strcmp(name, user[i])==0)
                        {
							continue;
						}
						else
                        {
							send(connfd[i], msg_send, strlen(msg_send), 0);
						}
					}
				}
			}
		}
	}
}

void Game(int p1,int p2,int num)
{
  int j,k,z;
  int col=3;
  int row=3;
  char ans=0;
  char ret=0;

  //initial the game
  
    for(j=0;j<row;j++)
    {
	  for(k=0;k<col;k++)
      {
	    array[num][j][k]='N';
	  }
    }


  for(z=0;z<10;z++)
  {
    if(z==num)
    {
	  printf("第%d個連線遊戲\n",z);
	  displayboard(z);
	}
  }
  
  while(1)
  {
	//player1 playing
	ans=move(p1,'O',ret,num);
	if(ans!=' ')
    {
	  break;
	}
	send(connfd[p1], "<Server> waiting...\n", strlen("<Server> waiting...\n"),0);
	
	//player2 playing
	ans=move(p2,'X',ret,num);
	if(ans!=' ')
    {
	  break;
	}
	send(connfd[p2], "<Server> waiting...\n", strlen("<Server> waiting...\n"),0);
  }
  if(ans == 'O')
  {
	send(connfd[p1], "<Server> You win the game!\n", strlen("<Server> You win the game!\n"),0);
	send(connfd[p2], "<Server> You lose the game!\n", strlen("<Server> You lose the game!\n"),0);
  }
  else if(ans == 'X')
  {
	send(connfd[p2], "<Server> You win the game!\n", strlen("<Server> You win the game!\n"),0);
	send(connfd[p1], "<Server> You lose the game!\n", strlen("<Server> You lose the game!\n"),0);
  }
  else if(ans == 'Q')
  {
	send(connfd[p1], "<Server> Tie!\n", strlen("<Server> Tie!\n"),0);
	send(connfd[p2], "<Server> Tie!\n", strlen("<Server> Tie!\n"),0);
  }  
} 

void displayboard(int num)
{
  int l=0,m=0;
  for(l=0; l<row; l++)
  {                                
	for(m=0; m<col; m++)
    {
		printf(" %c ", array[num][l][m]);
		if(m<col-1)                                
			printf("|");                           
	}
	printf("\n");
	if(l<row-1)
    {                                     
	  for(m=0; m<col; m++)
      {
		printf("---");
		  if(m<col-1)
			printf("|");
	  }
	}
	printf("\n");
  }
}

char iswin(char array[10][row][col],int num)  //判斷勝負
{         
  int j=0;
  for(j=0; j<row; j++)
  {                                //判斷行  是否能勝利
	if(array[num][j][0]==array[num][j][1] && array[num][j][1]==array[num][j][2] && array[num][j][0]!='N')
    {
	  return array[num][j][0];                           //若三子相同,且不能為空，返回元素內值
	}
  } 
  for(j=0; j<col; j++)
  {                                //判斷列  是否能勝利
    if(array[num][0][j]==array[num][1][j] && array[num][1][j]==array[num][2][j] && array[num][1][j]!='N')
    {
	  return array[num][1][j];
	}
  }
  if(array[num][0][0]==array[num][1][1] && array[num][1][1]==array[num][2][2] && array[num][1][1] != 'N')
  {   //判斷正對角線能否勝利
	return array[num][1][1];
  }
  if(array[num][0][2]==array[num][1][1] && array[num][1][1]==array[num][2][0] && array[num][1][1] != 'N')
  {   //判斷反對角線能否勝利
	return array[num][1][1];
  }
  if(IsFull(array,num))                                         //判定平局
  {
	return 'Q';
  }
  return  ' ';                                     //繼續
}

static int IsFull(char arr[10][row][col],int num)   //不為介面函式，不需要放入game.h，僅在此使用，無法進行test.c中呼叫
{  
  int l = 0;
  int m = 0;
  for(l=0; l<row; l++)
  {                                //遍歷二維陣列
    for(m=0; m<col; m++)
    {
	  if(array[num][l][m] == 'N')
      {                        //判斷是否仍有空值
		return 0;                               //有空值返回0
	  }
	}
  }
  return 1;                                           //無空值返回1
}

int move(int player,char op,char ret,int num)  //傳現在是哪個player給他跟他代表的符號（O or X)
{ 
  int l,m;
  int length;
  int x,y;
  char chosen[MAXLINE];
  memset(chosen,'\0',sizeof(chosen));
  send(connfd[player], "<Server> it's your turn\n", strlen("<Server> it's your turn\n"),0);
  while(1)
  {
    x=-1;
	y=-1;
	memset(chosen,'\0',sizeof(chosen));
	length=0;
    length = recv(connfd[player], chosen, MAXLINE, 0);
    chosen[length]=0;
    x=((int)chosen[0])-48;//minus 0 ASCII
    y=((int)chosen[2])-48;
	if(array[num][x][y]=='N')
    {//此格尚未填過
	  break;
	}
	if(x>2||x<0||y>2||y<0)
    {
	  send(connfd[player], "<Server> Error,out of range.\n", strlen("<Server> Error,out of range.\n"),0);
	  continue;
	}
	else
    {
	  send(connfd[player], "<Server> Error,this place is chosen.\n", strlen("<Server> Error,this place is chosen.\n"),0);
	  continue;
	}
  }
  printf("%d %d\n",x,y);
  for(l=0;l<3;l++)
  {
    for(m=0;m<3;m++)
    {
      if(l==x&&m==y&&array[num][l][m]=='N')
      {
    	array[num][l][m]=op;
	  }
	}
  }
  printf("第%d個連線遊戲\n",num);
  displayboard(num);
  ret=iswin(array,num);
  if(ret != ' ')
  { //如果此二維陣列內元素未定義，即為空，在執行下面的電腦走
    printf("遊戲結束！\n");
    return ret;
  }
  return ret;
}

int main()
{
	pthread_t thread;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t length;
	char buff[MAXLINE];

//socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(listenfd < 0)
    {
		printf("Socket created failed.\n");
		return -1;
	}
//set
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);	//port80
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//bind
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
		printf("Bind failed.\n");
		return -1;
	}
//listen
	printf("listening...\n");
	listen(listenfd, LISTENQ);

//建立thread管理server
	pthread_create(&thread, NULL, (void*)(&Quit), NULL);

//紀錄閒置的client(-1)
//initialize
	int i=0;
	for(i=0; i<MAXMEM; i++)
    {
		connfd[i]=-1;
	}
	memset(user, '\0', sizeof(user));
	printf("initialize...\n");

	while(1)
    {
		length = sizeof(cli_addr);
		for(i=0; i<MAXMEM; i++)
        {
			if(connfd[i]==-1)
            {
				break;
			}
		}
//wait client
		printf("receiving...\n");
		connfd[i] = accept(listenfd, (struct sockaddr*)&cli_addr, &length);

//對新client建thread，以開啟訊息處理
		pthread_create(malloc(sizeof(pthread_t)), NULL, (void*)(&rcv_snd), (void*)i);
	}

	return 0;
}