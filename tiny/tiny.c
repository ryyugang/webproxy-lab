/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

// HTTP request 처리하는 함수
void doit(int fd); 
// HTTP request Header 읽는 함수
void read_requesthdrs(rio_t *rp);
// URI 파싱하는 함수
int parse_uri(char *uri, char *filename, char *cgiargs);
// 정적 콘텐츠 제공하는 함수
void serve_static(int fd, char *filename, int filesize);
// 파일 이름으로부터 파일 타입을 결정하는 함수
void get_filetype(char *filename, char *filetype);
// 동적 콘텐츠 제공하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs);
// 클라이언트 오류 처리 함수
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    // 포트 번호 받기
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 주어진 포트 번호로 listsening socket 생성
  listenfd = Open_listenfd(argv[1]);
  
  // listening socket은 계속 수신 대기 상태
  while (1) {
    clientlen = sizeof(clientaddr);
    // request 수신 시, Accept() 함수를 통해 통신 소켓을 생성하여 연결
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    // request 처리하는 함수 doit()
    doit(connfd);   // line:netp:tiny:doit
    // request 처리 이후 연결 종료
    Close(connfd);  // line:netp:tiny:close
  }
}

// HTTP request 처리하는 함수
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd); // 읽기 버퍼 rio 생성 및 초기화 + 입력 인자로 받은 통신 소켓 (connfd)과 연결
  Rio_readlineb(&rio, buf, MAXLINE); // 읽기 버퍼 rio에서 request line 을 읽어 buf에 저장
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version); // request line 에서 3개의 인자를 추출

  // request method가 GET이 아니면 예외 처리
  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio); // request Header 읽기

  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs); // URI 파싱하여 정적 컨텐츠 여부 확인

  // 파일 정보를 가져오고, 파일이 없다면 예외 처리
  if (stat(filename, &sbuf) < 0)
  {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't read the file");
    return;
  }

  /* Serve static content */
  // 정적 컨텐츠인 경우
  if (is_static)
  {
    // 파일 읽기 권한이 있는지 확인하기
    // S_ISREG : 일반 파일 여부, S_IRUSR : 읽기 권한 여부, S_IXUSR : 실행 권한 여부

    // 일반 파일이 아니고, 읽기 권한이 없다면 예외처리
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size); // 일반 파일이 맞고, 읽기 권한이 있다면 통신 소켓에 파일 전달
  }
  /* Serve dynamic content */
  // 동적 컨텐츠일 경우
  else
  {
    // 일반 파일이 아니고, 실행 권한이 없다면 예외 처리
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs); // 일반 파일이 맞고, 실행 권한이 있다면 통신 소켓에 파일 전달
  }
}

// 클라이언트 오류 처리 함수
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF]; // response Header, body 저장 버퍼 선언

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny error</title>"); // response body에 HTML 제목 추가
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body); // response body에 배경색 지정
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg); // response body에 에러 번호와 짧은 메세지 추가
  sprintf(body, "%s<p>%s : %s\r\n", body, longmsg, cause); // response body에 상세한 에러 메세지 추가
  sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body); // response body에 수평선과 웹 서버 이름 추가

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg); // response Header에 HTTP 버전, 에러 번호, 짧은 메세지 추가
  Rio_writen(fd, buf, strlen(buf)); // response Header를 통신 소켓에 전달
  sprintf(buf, "Content-type: text/html\r\n"); // response Header에 컨텐츠 유형 추가
  Rio_writen(fd, buf, strlen(buf)); // response Header를 통신 소켓에 전달
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body)); // response Header에 컨텐츠 길이 추가
  Rio_writen(fd, buf, strlen(buf)); // response Header를 통신 소켓에 전달
  Rio_writen(fd, body, strlen(body)); // response body를 통신 소켓에 전달
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE]; // response Header 저장 버퍼 선언

  Rio_readlineb(rp, buf, MAXLINE); // 입력받은 버퍼(&rio)에서 request Header 읽고 buf에 저장
  while(strcmp(buf, "\r\n")) // buf가 빈 줄을 만날 때까지 읽기
  {
    Rio_readlineb(rp, buf, MAXLINE); 
    printf("%s", buf);
  }
  return;
}

// URI 파싱하는 함수
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  /* Static content */

  // cgi-bin이 없다면 -> 정적 컨텐츠를 위한 것이라면
  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, ""); // cgi 인자 문자열을 지운다
    strcpy(filename, "."); // 파일 이름 문자열을 .으로 바꾼다
    strcat(filename, uri); // 파일 이름 문자열에 URI를 추가한다 -> [./home.html] 된다
    
    // URI가 /로 끝난다면
    if (uri[strlen(uri) - 1] == '/') 
    {
      strcat(filename, "home.html"); // 파일 이름 문자열을 home.html로 바꾼다
    }
    return 1; // 1 리턴 -> 정적 컨텐츠 !
  }
  /* Dynamic content */

  // cgi-bin이 있다면 -> 동적 컨텐츠를 위한 것이라면
  else
  {
    ptr = index(uri, '?'); // URI에서 ?의 위치를 찾는다
    
    // URI에 ? 존재하는 경우
    if (ptr)
    {
      strcpy(cgiargs, ptr+1); // CGI 인자 추출하여 cgiargs에 저장
      *ptr = '\0'; // URI에서 CGI 인자 제거 (ptr이 CGI 인자를 가리키고 있었으니까, ptr이 가리키는 곳부터 NULL로 만드는 것)
    }
    
    // URI에 ? 존재하지 않는 경우
    else
    {
      strcpy(cgiargs, ""); // CGI 인자 초기화
    }
    strcpy(filename, "."); // 파일 이름 문자열을 .로 바꾼다
    strcat(filename, uri); // 파일 이름 문자열에 URI 추가한다 -> [./home.html] 된다
    return 0; // 0 리턴 -> 동적 컨텐츠 !
  }
}

// 정적 컨텐츠 제공하는 함수
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd; // 파일 디스크립터 저장 변수
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype); // 파일 이름으로부터 파일 타입 추출
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // response 버퍼에 상태 코드와 상태 메세지 저장
  sprintf(buf, "%sServer : Tiny Web Server\r\n", buf); // response 버퍼에 서버 정보 추가
  sprintf(buf, "%sConnection : close\r\n", buf); // responser 버퍼에 연결 종료 정보 추가
  sprintf(buf, "%sContent-length : %d\r\n", buf, filesize); // response 버퍼에 컨텐츠 길이 추가
  sprintf(buf, "%sContent-type : %s\r\n\r\n", buf, filetype); // response 버퍼에 파일 타입 추가
  Rio_writen(fd, buf, strlen(buf)); // 통신 소켓에 response Header 전송
  printf("Response headers:\n"); 
  printf("%s", buf); // 서버 측에 response Header 출력

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // 읽기 전용으로 파일을 open
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 파일을 가상메모리에 매핑
  Close(srcfd); // 파일 디스크립터 닫기
  Rio_writen(fd, srcp, filesize); // 통신 소켓에 주소 srcp에서 시작하는 filesize 바이트 복사
  Munmap(srcp, filesize); // Mmap으로 매핑된 가상메모리 주소를 반환
}

/*
* get_filetype : Derive file type from filename
*/

// 파일 이름으로부터 파일 타입을 결정하는 함수
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
  {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif"))
  {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else
  {
    strcpy(filetype, "text/plain");
  }
}

// 동적 컨텐츠 제공하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL }; 

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // response 버퍼에 상태 코드와 상태 메세지 젖장
  Rio_writen(fd, buf, strlen(buf)); // 통신 소켓에 response Header 전송
  sprintf(buf, "Server : Tiny Web Server\r\n"); // response 버퍼에 서버 정보 추가
  Rio_writen(fd, buf, strlen(buf)); // 통신 소켓에 response Header 전송

  /* Child */
  // 자식 프로세서를 생성
  if (Fork() == 0)
  {
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1); // 환경 변수를 CGI 인자로 설정
    /* Redirect stdout to client */
    
    // (Duplicate), STDOUT_FILENO : 표준 출력(stdout)
    // 자식 프로세스의 출력이 통신 소켓 (Client와 연결된)으로 연결되는 것
    Dup2(fd, STDOUT_FILENO); // 자식 프로세스의 표준 출력을 통신 소켓과 연결
    /* Run CGI program */
    Execve(filename, emptylist, environ); // CGI 프로그램 실행
  }
  /* Parent waits for and reaps child */
  Wait(NULL); // 자식 프로세스가 종료될 때까지 대기
}