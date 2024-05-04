#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd; // 서버와 통신을 위한 클라이언트 소켓 선언
    char *host, *port, buf[MAXLINE]; // 호스트 명을 가리키는 포인터, 포트 번호를 가리키는 포인터, 버퍼 사이즈는 최대로
    rio_t rio; // RIO 패키지 읽기 버퍼 선언

    if (argc != 3) // 정상적인 입력이 아닐 경우
    {
        fprintf(stderr, "usage : %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1]; // 호스트 명
    port = argv[2]; // 포트 번호

    clientfd = Open_clientfd(host, port); // 주어진 호스트 명과 포트 번호를 클라이언트 소켓에 저장
    Rio_readinitb(&rio, clientfd); // 읽기 버퍼 rio 초기화 + 서버와 연결될 클라이언트 소켓(clientfd)과 연결

    while (Fgets(buf, MAXLINE, stdin) != NULL) // 사용자로부터 입력을 받아 buf에 저장
    {
        Rio_writen(clientfd, buf, strlen(buf)); // buf에 저장된 데이터를 서버로 전송
        Rio_readlineb(&rio, buf, MAXLINE); // 서버로부터 응답을 받아 buf에 저장
        Fputs(buf, stdout); // 서버로부터 받은 응답을 표준출력(stdout)
        printf("==================================\n");
    }

    close(clientfd);
    exit(0);
}