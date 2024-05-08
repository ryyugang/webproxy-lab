#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd); 
void p_request(int clientfd, char *host, char *port, char *path, char *method);
void p_response(int proxy_connfd, int clientfd);
int parse_uri(char *uri, char *host, char *port, char *path);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv) {
  int listenfd, proxy_connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
  
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 주어진 포트 번호로 listening socket 생성
  listenfd = Open_listenfd(argv[1]);

  // listening socket은 계속 수신 대기 상태
  while (1) {
    clientlen = sizeof(clientaddr);
    // request 수신 시, Accept() 함수를 통해 통신 소켓을 생성하여 연결
    proxy_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(proxy_connfd);  
    Close(proxy_connfd); 
  }
}

void doit(int proxy_connfd)
{
  int clientfd;
  char buf[MAXLINE], host[MAXLINE], path[MAXLINE], port[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rio;

  /* Read request line and headers from Client */
  Rio_readinitb(&rio, proxy_connfd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers to proxy:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // GET http://www.cmu.edu/hub/index.html HTTP/1.1

  if (parse_uri(uri, host, port, path) < 0)
  {
    clienterror(proxy_connfd, uri, "404", "Not found", "Tiny couldn't parse the request");
    return;
  }

  clientfd = Open_clientfd(host, port);
  p_request(clientfd, host, port, path, method);
  p_response(proxy_connfd, clientfd);
  Close(clientfd);
}

// proxy -> server
void p_request(int clientfd, char *host, char *port, char *path, char *method)
{
  char buf[MAXLINE];

  printf("Request headers to server:\n");
  printf("%s %s %s\n", method, path, "HTTP/1.0");

  // request Header 
  sprintf(buf, "%s %s %s \r\n", method, path, "HTTP/1.0");
  sprintf(buf, "%sHost: %s \r\n", buf, host);
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sProxy-connection: close\r\n\r\n", buf);

  Rio_writen(clientfd, buf, (size_t)strlen(buf));
}

// server -> proxy
void p_response(int proxy_connfd, int clientfd)
{
  char buf[MAXLINE];
  ssize_t n;
  rio_t rio;

  Rio_readinitb(&rio, clientfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
  {
    printf("Proxy received %ld bytes from server\n", n);
    Rio_writen(proxy_connfd, buf, n);
  }
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s : %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

// http://www.cmu.edu/hub/index.html
int parse_uri(char *uri, char *host, char *port, char *path)
{
  char *ptr;

  if (!(ptr = strstr(uri, "://"))) 
  {
    return -1;
  }
  ptr += 3;
  strcpy(host, ptr); // host = www.cmu.edu/hub/index.html
  
  if((ptr = strchr(host, '/'))){  
    *ptr = '\0'; // host = www.cmu.edu
    ptr += 1;
    strcpy(path, "/"); // path = /
    strcat(path, ptr); // path = /hub.index.html           
  }
  else 
  {
    strcpy(path, "/");
  }

  if ((ptr = strchr(host, ':'))){ 
    *ptr = '\0'; // 포트 있으면 port에 넣기    
    ptr += 1;     
    strcpy(port, ptr);                
  }  
  else
  {
    strcpy(port, "80"); // 포트 없으면 80 지정
  }

  return 0;
}