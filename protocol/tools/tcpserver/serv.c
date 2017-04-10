#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>


#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)

void upper(char* buf) {
  char* p = buf;
  while(*p) {
    *p = toupper(*p);
    ++p;
  }
}

/*
 * ./serv <ip> <port> [SO_LINGER]
 *
 */

int so_linger = 0;

void setopt(int sockfd) {
  int ret;
  if (so_linger) {
    struct linger slinger;
    slinger.l_onoff = 1; // 关闭连接时发送 RST 段
    slinger.l_linger = 0;// 不等待
    ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &slinger, sizeof(slinger));
    if (ret < 0) ERR_EXIT("set SO_LINGER failed");
  }
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <ip> <port> [SO_LINGER]\n", argv[0]);
    return 1;
  }

  if (argc >= 4) {
    if (!strcmp("SO_LINGER", argv[3])) {
      so_linger = 1;
    }
  }

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
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) ERR_EXIT("socket");
  // set option
  setopt(sockfd);

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
    n = read(clientfd, buf, 64);
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
    upper(buf);
    write(clientfd, buf, n);
  }
  close(clientfd);
  close(sockfd);

  return 0;
}
