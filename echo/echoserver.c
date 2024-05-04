#include "csapp.h"

// 클라이언트와의 통신 처리 + 클라이언트가 보낸 데이터 수신 -> 송신 == echo method
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE]; // 수신한 데이터를 저장할 buf 선언, 크기는 최대로
    rio_t rio; // RIO 패키지 읽기 버퍼 rio 선언

    Rio_readinitb(&rio, connfd); // 읽기 버퍼 rio 초기화 + 클라이언트와 연결된 소켓과 연결
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) // rio 버퍼에서 한 줄씩 데이터 읽어오기 / 0이 아닐 때까지 반복
    {
        printf("server received: %s", buf); //  서버가 수신한 데이터 출력
        printf("server received %d bytes\n", (int)n); // 서버가 수신한 데이터의 바이트 수 출력
        Rio_writen(connfd, buf, n); // [수신한 데이터가 저장된 buf]의 데이터를 클라이언트와 연결된 소켓[connfd]을 통해 클라이언트로 송신
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd; // 수신 대기 상태 전용 소켓[listenfd], 클라이언트와 연결될 소켓[connfd] 선언
    socklen_t clientlen; // 클라이언트 주소 구조체의 크기를 저장할 변수 선언
    struct sockaddr_storage clientaddr; // 클라이언트 주소 정보를 저장할 구조체 선언
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트의 호스트명과 포트 번호를 저장할 배열 선언

    if (argc != 2) // 여기서 argc는 클라이언트의 호스트 명과 포트 번호, 정상적 입력이 아닐 경우의 예외처리
    {
        fprintf(stderr, "usage %s <port>\n", argv[0]); 
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]); // 주어진 포트 번호로 서버 소켓 생성, 반환된 파일 디스크립터를 수신 대기 상태 전용 소켓[listenfd]에 저장
    while (1) // 클라이언트와의 연결은 무한루프
    {
        clientlen = sizeof(struct sockaddr_storage); // 클라이언트 주소 구조체의 크기 저장
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트의 연결 요청 수락, 연결된 소켓의 파일 디스크립터를 connfd에 저장, 클라이언트 주소 정보는 clientaddr에 저장
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // clientaddr에 저장된 클라이언트 주소 정보에서 [호스트명, 포트 번호] 추출하여 저장
        printf("Connected to (%s, %s)\n", client_hostname, client_port); // 연결된 클라이언트의 호스트명과 포트 번호 출력
        echo(connfd); // echo() 호출, 입력 인자로 클라이언트와 연결된 소켓(connfd)
        Close(connfd); // 클라이언트와 연결된 소켓 (connfd) 종료
    }
    exit(0);
}