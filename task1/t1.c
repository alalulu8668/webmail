/*******HTTP*****/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc,char *argv[] )
{
	int serversock;
	int clientsock;
	struct sockaddr_in clientaddr;	
	struct sockaddr_in seraddr;
	int len;
	char buf[BUFSIZ];
	
	memset(&seraddr,0,sizeof(seraddr));//data init, set 0
	
	seraddr.sin_family = AF_INET;//ipv4
	seraddr.sin_addr.s_addr = INADDR_ANY;
	seraddr.sin_port = htons(8000);
	
	/****create socket, TCP ****/
	if((serversock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}
	
	/*****bind the socket to the internet*****/
	if(bind(serversock, (struct sockaddr*)&seraddr, sizeof(struct sockaddr)) < 0)
	{	
		perror("bind");
		return 1;	
	}

	/*****listen to the socket****/
	listen(serversock, 5);
	int sin_size = sizeof(struct sockaddr_in);

	/******wait for client*******/
	if((clientsock = accept(serversock, (struct sockaddr *)&clientaddr,&sin_size)) < 0)
	{
		perror("accept");
		return 1;
	}
	printf("accept client %s/n", inet_ntoa(clientaddr.sin_addr));
	
	/**************send the message************/
	char *msg = "welcome to leiwanlu/n";
	int lenth;
	lenth = strlen(msg);
	len = send(clientsock,msg,lenth,0);
	
	/************receive message************/
	while((len = recv(clientsock, buf, BUFSIZ, 0)) > 0)
	{
		printf("lalalala");
		buf[len] = '/0';
		printf("%s/n",buf);
		if(send(clientsock,buf,len,0) < 0)
		{	
			perror("write");
			return 1;
		}
	}

	close(clientsock);
	close(serversock);
	 
} 

