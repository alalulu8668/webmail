#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <cstring>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <resolv.h>


using namespace std;

#define BACKLOG 3
#define BUFSIZE 1024*4
#define HELO "HELO youbei.jin\r\n"
#define DATA "DATA\r\n"


const char *notfound = "HTTP/1.1 404 Not Found\r\n"       "Content-Type: text/html\r\n"       "Content-Length: 40\r\n"        "\r\n"             "<HTML><BODY>File not found</BODY></HTML>";
const char *badrequest = "HTTP/1.1 400 Bad Request\r\n"       "Content-Type: text/html\r\n"       "Content-Length: 39\r\n"        "\r\n"             "<h1>Bad Request (Invalid Hostname)</h1>";

char *head_analysis(char *p);
void read_http(const char *p, char *pbuf, int conn);
void smtp(char *data, int http);
void send_s(const char *s);
void read_s();
char *mx (char *string);
std::string qp(const std::string& src);
char *replace(char *st, char *before, char *after);

/****************From cgicc lib, for url decoding********************/
char hexToChar(char first, char second)
{
  int digit;

  digit = (first >= 'A' ? ((first & 0xDF) - 'A') + 10 : (first - '0'));
  digit *= 16;
  digit += (second >= 'A' ? ((second & 0xDF) - 'A') + 10 : (second - '0'));
  return static_cast<char>(digit);
}

std::string form_urldecode(const std::string& src)
{
  std::string result;
  std::string::const_iterator iter;
  char c;

  for(iter = src.begin(); iter != src.end(); ++iter) {
    switch(*iter) {
    case '+':
      result.append(1, ' ');
      break;
    case '%':
      // Don't assume well-formed input
      if(std::distance(iter, src.end()) >= 2
         && std::isxdigit(*(iter + 1)) && std::isxdigit(*(iter + 2))) {
        c = *++iter;
        result.append(1, hexToChar(c, *++iter));
      }
      // Just pass the % through untouched
      else {
        result.append(1, '%');
      }
      break;

    default:
      result.append(1, *iter);
      break;
    }
  }
  return result;
}
/*************** From cgicc lib, for url decoding *******************/


/**************** HTTP ***********************/
int main(void)
{
    int lsn;
    struct sockaddr_in servAddr;
    unsigned port = ntohs(servAddr.sin_port);
    printf("using port %d\n", port);
    lsn = socket(AF_INET, SOCK_STREAM, 0);
    // reuse port
    int optval = 1;
    setsockopt(lsn, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(lsn, (struct sockaddr *)&servAddr, sizeof(struct sockaddr)) != 0)
    {
        close(lsn);
        printf("binding error\n");
        return 1;
    }
    printf("binding port %d done\n", port);
    listen(lsn, BACKLOG) != 0;

    while(1)
    {
        pid_t pid;
        struct sockaddr_in cliAddr;
        size_t cliAddrLen = sizeof(cliAddr);
        char *p = NULL,
            fromm[20],
            filen[9999] = "index.html",
            rcvbuf[9999],
            sendbuf[BUFSIZE];

        int conn = accept(lsn, (struct sockaddr *)&cliAddr, (socklen_t*)&cliAddrLen);
        pid = fork();
        if(pid == 0)
        {
            close(lsn);
            strcpy(fromm, inet_ntoa(cliAddr.sin_addr));
            printf("\n\nclient %s connected.\n", fromm);

            if(recv(conn, rcvbuf, sizeof(rcvbuf), 0) > 0)
            {
                p = head_analysis(rcvbuf);
                printf("%s\n\n", p);
            }
            else
                exit(0);

            if(p != NULL)
            {
                                                        if(strncmp(p, "?", 1) == 0)
                                                        {
                                                                if(strlen(p) > 2000)
                                                                                send(conn, "Your email is too long", strlen("Your email is too long"), 0);
                                                                else
                                                                                smtp(p, conn);
                                                        }
                                                        else
                                                        {
                strcat(filen, p);
                read_http(filen, sendbuf, conn);
                                                        }
            }
            else
                send(conn, badrequest, strlen(badrequest), 0);

            exit(0);
        }
        else if(pid > 0)
            close(conn);
        else
            return 1;
    }
    close(lsn);

    return 0;
}

char *head_analysis(char *p)
{
    char *ptmp = p;

    if(strncmp(ptmp, "GET /", 5) == 0)
    {
        ptmp += 5;
        while(*ptmp != ' ')
            ptmp++;
        *ptmp++ = '\0';

        if(strncmp(ptmp, "HTTP/", 5) == 0)
            return p+5;
        else
            return NULL;
    }
    else
        return NULL;
}

void read_http(const char *p, char *pbuf, int conn)
{
    int fd;
    size_t rdLen;
    fd = open(p, O_RDONLY);
    if(fd != -1)
    {
        send(conn, "HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n", strlen("HTTP/1.1 200 OK\r\nContent-Type:text/html\r\n\r\n"), 0);
        while((rdLen = read(fd, pbuf, BUFSIZE)) > 0)
            send(conn, pbuf, rdLen, 0);
        printf("write to client finished\n\n\n");
        close(fd);
    }
    else
        send(conn, notfound, strlen(notfound), 0);
}

/**************** HTTP ***********************/

/*********************** SMTP *************************/

FILE *fin;
int sock;
struct sockaddr_in server;
struct hostent *hp, *gethostbyname();
char buf[BUFSIZ+1];
int len;
char wkstr[100];

void smtp(char *data, int http)
{
        char decoded[9999];
        char *q;

        string data1 = form_urldecode(data);
        int stringsize = data1.size();
        for (int a=0; a<=stringsize; a++)
        {
                decoded[a] = data1[a];
        }

        char *from = strtok(decoded, "&");
        from += 6;

        char *to = strtok(NULL, "&");
        to += 3;

        char *subject = strtok(NULL, "&");
        subject += 8;

        char *body = strtok(NULL, "&");
        body += 5;

        char *smtpserver;
        q = strtok(NULL, "&");
        if (strcmp(q, "smtpserver=") == 0)
        {
                char tooo[50];
                strcpy(tooo, to);
                smtpserver = mx(tooo);
                                                  }
        else
        {
                smtpserver = q;
                smtpserver += 11;
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);

        server.sin_family = AF_INET;
        hp = gethostbyname(smtpserver);
        if (hp==(struct hostent *) 0)
        {
                fprintf(stderr, "%s: unknown smtp server\n", smtpserver);
                                        send(http, "unknown smtp server", strlen("unknown smtp server"), 0);
                exit(2);
        }

        //Connect to port 25
        memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
        server.sin_port=htons(25);
        connect(sock, (struct sockaddr *) &server, sizeof server)==-1;

        //login info
        read_s();
  //send(http, buf, strlen(buf), 0);

        //my name is
        send_s(HELO);
        read_s();
  //send(http, buf, strlen(buf), 0);

        //my address
        send_s("MAIL from: <");
        send_s(from);
        send_s(">\r\n");
        read_s();
  //send(http, buf, strlen(buf), 0);

        //to address
        send_s("RCPT To: <");
        send_s(to);
                                         send_s(">\r\n");
        read_s();
  //send(http, buf, strlen(buf), 0);

        send_s(DATA);
        read_s();
  //send(http, buf, strlen(buf), 0);

        //=?iso-8859-1?Q?encoded text?= 
        //subject
        send_s("Subject:=?utf-8?Q?");
        send_s(&qp(subject)[0]);
        send_s("?=\r\n");

        //body
        send_s("MIME-Version: 1.0\r\n");
        send_s("Content-Type: text/plain; charset=utf-8\r\n");
        send_s("Content-Transfer-Encoding: quoted-printable\r\n");
        if (strncmp(body, ".", 1) == 0)
                                send_s(&qp(replace(body, ".", ".."))[0]);
        else
                                send_s(&qp(body)[0]);
        send_s("\r\n.\r\n");
        read_s();
        if(strncmp(buf, "250", 3) == 0)
                                send(http, "Your email has been sent!", strlen("Your email has been sent!"), 0);
        else
                                send(http, "Sending error!", strlen("Sending error!"), 0);
        //send(http, buf, strlen(buf), 0);

        close(sock);
        exit(0);
}

void send_s(const char *s)
{
        write(sock,s,strlen(s));
        write(1,s,strlen(s));
}

void read_s()
{
                           len = read(sock,buf,BUFSIZ);
        write(1,buf,len);
}

char *replace(char *st, char *before, char *after)
{
          static char buf[4096];
          char *ch;
          if (!(ch = strstr(st, before)))
                 return st;
          strncpy(buf, st, ch-st);
          buf[ch-st] = 0;
          sprintf(buf+(ch-st), "%s%s", after, ch+strlen(before));
          return buf;
}


/*********************** SMTP *************************/

/********************** MX ****************************/
//g++ mx.c -lresolv -o mx

char *mx (char *string)
{

        char t[2][50];
        char *q;
        int k=0;

        q = strtok(string, "@");
        while (q != NULL)
        {
                strcpy(t[k++], q);
                q = strtok(NULL, "@");
        }


        u_char nsbuf[9999];
        char dispbuf[9999];
        ns_msg msg;
        ns_rr rr;
        int j, l;
        l = res_query (t[1], ns_c_any, ns_t_mx, nsbuf, sizeof (nsbuf));
        if (l < 0)
        {
            perror (t[1]);
        }
        else
        {
            ns_initparse (nsbuf, l, &msg);
            l = ns_msg_count (msg, ns_s_an);
            for (j = 0; j < l; j++) {
                ns_parserr (&msg, ns_s_an, j, &rr);
                ns_sprintrr (&msg, &rr, NULL, NULL, dispbuf, sizeof (dispbuf));
            }
        }
//      ik2213.lab.\t\t2h52m12s IN MX\t0 mail.ik2213.lab.
        char *p;
        char s[4][30];
        int i=0;
        p = strtok(dispbuf, " ");
         while (p != NULL)
          {
               strcpy(s[i++], p);
                 p = strtok(NULL, " ");
          }
         char dest[] = "";
         strncat(dest, s[3], strlen(s[3])-1);
         return dest;
}

/********************** MX ****************************/

/*************** Quoted-printable *********************/

std::string qp(const std::string &src)
{
        string dst;
        for (int i = 0; i < src.size(); i++)
        {
                if ((src[i] >= '!') && (src[i] <= '~') && (src[i] != '=') && (src[i] != '?'))

                {
                        dst += src[i];//do nothing
                                                        }
                else
                {
                        char char1 = 0x0F & (src[i] >> 4);
                        char char2 = 0x0F & src[i];

                        dst += '=';
                        dst += (char1 < 0xA) ? (char1 + 48):(char1 + 55);
                        dst += (char2 < 0xA) ? (char2 + 48):(char2 + 55);
                }
        }
        return dst;
}

/*************** Quoted-printable *********************/

                                                              
