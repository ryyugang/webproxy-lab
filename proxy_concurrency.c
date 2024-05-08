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
void *thread(void *vargp); // pthread 사용을 위한 함수

int main(int argc, char **argv) {
  int listenfd, *proxy_connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 주어진 포트 번호로 수신 소켓 생성
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    
    // 연결된 클라이언트를 위한 메모리 할당
    proxy_connfd = Malloc(sizeof(int));
    
    // 클라이언트로부터 연결 요청 수락
    *proxy_connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
    
    // 연결된 클라이언트의 호스트 이름과 포트 번호 가져오기
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
  
    // 클라이언트 요청 처리를 Thread로 
    Pthread_create(&tid, NULL, thread, proxy_connfd);
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

  // URI 파싱하여 호스트, 포트, 경로 추출
  if (parse_uri(uri, host, port, path) < 0)
  {
    clienterror(proxy_connfd, uri, "404", "Not found", "Tiny couldn't parse the request");
    return;
  }

  // 서버와의 연결 설정
  clientfd = Open_clientfd(host, port);
  
  // Proxy -> Server request 전송
  p_request(clientfd, host, port, path, method);

  // Server -> Proxy response 전송
  p_response(proxy_connfd, clientfd);

  // 연결 종료
  Close(clientfd);
}

// Proxy -> Server request 전송 메소드
void p_request(int clientfd, char *host, char *port, char *path, char *method)
{
  char buf[MAXLINE];

  printf("Request headers to server:\n");
  printf("%s %s %s\n", method, path, "HTTP/1.0");

  // Request Header 읽기
  sprintf(buf, "%s %s %s \r\n", method, path, "HTTP/1.0");
  sprintf(buf, "%sHost: %s \r\n", buf, host);
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sProxy-connection: close\r\n\r\n", buf);

  // Server로 HTTP request 전송
  Rio_writen(clientfd, buf, (size_t)strlen(buf));
}

// Server -> Proxy response 전송 메소드
void p_response(int proxy_connfd, int clientfd)
{
  char buf[MAXLINE];
  ssize_t n;
  rio_t rio;

  // 서버로부터 response 읽기
  Rio_readinitb(&rio, clientfd);
  while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
  {
    printf("Proxy received %ld bytes from server\n", n);

    // Proxy로 HTTP response 전송
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

  // URI에 :// 없으면 예외처리 + http:// 이후부터 읽게 추출
  if (!(ptr = strstr(uri, "://"))) 
  {
    return -1;
  }
  ptr += 3;
  strcpy(host, ptr); // host = www.cmu.edu/hub/index.html

  
  // 호스트 이름 + 경로 추출
  if((ptr = strchr(host, '/'))){  
    *ptr = '\0';                      
    ptr += 1;
    strcpy(path, "/");            
    strcat(path, ptr);            
  }
  else 
  {
    strcpy(path, "/");
  }

  // 포트 번호 추출 + 없으면 80으로 고정
  if ((ptr = strchr(host, ':'))){     
    *ptr = '\0';                      
    ptr += 1;     
    strcpy(port, ptr);                
  }  
  else
  {
    strcpy(port, "80");            
  }

  return 0;
}

// Thread routine
void *thread(void *vargp)
{
  int proxy_connfd = *((int *)vargp);
  
  // thread 분리 (종료되면 자원 자동 해제되게)
  Pthread_detach(pthread_self());

  // 동적 할당된 메모리 해제
  Free(vargp);

  // 클라이언트 request 처리
  doit(proxy_connfd);

  // 연결 종료
  Close(proxy_connfd);
  return NULL;
}