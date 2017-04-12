#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>


#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)
char buf[1024];
int recvbufsize = 4096;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <ip> <port> [recvbuf size]\n", argv[0]);
    return 1;
  }
  if (argc >= 4) {
    recvbufsize = atoi(argv[3]);
  }

  struct sockaddr_in servaddr, cliaddr;
  int sockfd, clientfd, ret, n;
  socklen_t cliaddrlen;

  // 1. create sockaddr
  puts("1. create sockaddr");
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(atoi(argv[2]));

  // 2. create socket
  puts("2. create socket");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) ERR_EXIT("socket");
  ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbufsize, sizeof(recvbufsize));
  if (ret < 0) ERR_EXIT("setsockopt");
  n = sizeof(recvbufsize);
  ret = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbufsize, &n);
  if (ret < 0) ERR_EXIT("getsockopt");
  printf("actual recvbufsize: %d\n", recvbufsize);


  // 3. bind sockaddr
  puts("3. bind sockaddr");
  ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (ret < 0) ERR_EXIT("bind");

  // 4. listen
  puts("4. listen");
  ret = listen(sockfd, 5);
  if (ret < 0) ERR_EXIT("listen");

  // 5. accept connect
  puts("5. accept connect");
  cliaddrlen = sizeof(cliaddr);
  clientfd = accept(sockfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
  if (clientfd < 0) ERR_EXIT("accept");
  printf("client fd: %d\n", clientfd);
  printf("sockaddr: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

  
  // 6. getsockname, 打印新的 socket 绑定的套接字地址 
  puts("6. getsockname");
  cliaddrlen = sizeof(cliaddr);
  ret = getsockname(clientfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
  if (ret < 0) ERR_EXIT("getsockaddr");
  printf("sockaddr: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

  // 7. send data
  while(1) {
    usleep(1000*500);
    n = read(clientfd, buf, 1024);
    if (n == 0) {
      puts("peer closed");
      sleep(1);
      break;
    }
    else if (n < 0) {
      if (errno == EINTR) continue;
      else {
        ERR_EXIT("read error");
      }
    }
    write(STDOUT_FILENO, buf, n);
  }
  close(clientfd);
  close(sockfd);

  return 0;
}
