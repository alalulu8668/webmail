#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void)
{	
	int clientsock;
	int len;
	struct sockaddr_in clientaddr;
	char buf[BUFSIZ];
	memset(&clientaddr, 0, sizeof(clientaddr));
	
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_port = htons(8000);
	clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //old obsolete function, not work with ipv6, now use inet_pton
	
	/************create***********/
	if((clientsock = socket (PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return 1;
	}
	
	/**********connect*****************/
	if(connect(clientsock, (struct sockaddr*)&clientaddr, sizeof(struct sockaddr)) < 0)
	{
		perror("connect");
		return 1;
	}
	printf("connect to server /n");
	len = recv(clientsock, buf, BUFSIZ, 0);
		buf[len] = '/0'; //?? what is the meaning?
	printf("%s", buf);

  	/***********cycly send and receive message***********/
	while(1)
	{
		printf("enter string is to send:");
		scanf("%s",buf);
		if(!strcmp(buf,"quit"))
		 	break;
		len = send(clientsock, buf, strlen(buf),0);
		len = recv(clientsock, buf, BUFSIZ, 0);
 		buf[len] = '/0';
		printf("received %s/n",buf);
	}
	close(clientsock);
}


