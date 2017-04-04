#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)

void upper(char* buf) {
  char* p = buf;
  while(*p) {
    *p = toupper(*p);
    ++p;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 3) return 1;
  struct sockaddr_in servaddr, cliaddr;
  int sockfd, clientfd, ret, n;
  socklen_t cliaddrlen;
  char buf[64];

  // 1. create sockaddr
  puts("1. create sockaddr");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(atoi(argv[2]));

  // 2. create socket
  puts("2. create socket");
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) ERR_EXIT("socket");

  // 3. bind sockaddr
  puts("3. bind sockaddr");
  ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (ret < 0) ERR_EXIT("bind");

  while(1) {
    cliaddrlen = sizeof(cliaddr);
    n = recvfrom(sockfd, buf, 63, 0, (struct sockaddr*)&cliaddr, &cliaddrlen);
    printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    puts(buf);
    upper(buf);
    sendto(sockfd, buf, n, 0, (struct sockaddr*)&cliaddr, cliaddrlen);
  }

  close(sockfd);

  return 0;
}
