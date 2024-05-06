/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // 크기가 MAXLINE인 버퍼 3개 선언
  int n1=0, n2=0; 

  if ((buf = getenv("QUERY_STRING")) != NULL) // getenv 함수를 사용해 QUERY_STRING 값을 buf에 할당
  {
    // 쿼리 문자열에서 & 문자를 찾아 두 개의 인자로 분리
    // 첫 번째 인자는 arg1, 두 번째 인자는 arg2
    p = strchr(buf, '&'); // buf에서 & 문자의 위치를 찾아 p에 할당
    *p = '\0'; // buf에서 & 문자의 위치를 NULL로 만든다 -> 문자열이 두 부분으로 분리된다
    strcpy(arg1, buf); // buf 내용(분리된 앞 부분) 을 arg1에 복사
    strcpy(arg2, p+1); // p+1부터 시작하는 문자열을 (분리된 뒷 부분) arg2에 복사
    // arg1, arg2를 정수로 변환하여 n1, n2 변수에 저장
    n1 = atoi(arg1); 
    n2 = atoi(arg2);
  }

  /* Response body */
  sprintf(content, "QUERY_STRING=%s", buf); // buf의 값을 / 형식 문자열에 따라 (QUERY_STRING=) / content 버퍼에 저장
  sprintf(content, "Welcome to add.com : "); // 형식 문자열을 / content 버퍼에 저장
  sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content); // content 버퍼에 저장된 내용을 유지하면서 추가적인 내용을 덧붙임
  sprintf(content, "%sThe answer is : %d + %d = %d\r\n<p>", // content 버퍼에 이전 내용을 추가하고 n1, n2, 그리고 그들의 합을 저장
          content, n1, n2, n1 + n2); 
  sprintf(content, "%sThanks for visiting!\r\n", content); // content 버퍼에 저장된 내용을 유지하면서 추가적인 내용을 덧붙임

  /* HTTP response (Header) */
  printf("Connection : close\r\n"); // "Connection : close" 출력
  printf("Content-length : %d\r\n", (int)strlen(content)); // 문자열 버퍼인 content의 길이를 계산하여 "Content-length"와 함께 출력
  printf("content-type : text/html\r\n\r\n"); // "content-type : text/html" + 빈 줄 출력 [Header 부분의 끝 :\r\n\r\n]
  printf("%s", content); // content 문자열 버퍼 출력
  fflush(stdout); // 출력 버퍼를 비운다

  exit(0);
}