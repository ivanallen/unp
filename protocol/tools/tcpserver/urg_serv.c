#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>


#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)
char buf[2048];
int recvbufsize = 4096;
int print = 0;
int g_clientfd;

void handler(int sig) {
	int n;
	if (sig == SIGURG) {
		puts("Hello SIGURG");
    n = recv(g_clientfd, buf, sizeof(buf) - 1, MSG_OOB);
		if (n < 0) {
			perror("recv");
			return;
		}
		buf[n] = 0;
		printf("read %d OOB byte: %s\n", n ,buf);
	}
}


int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s <ip> <port> [recvbuf size] [print]\n", argv[0]);
    return 1;
  }
  if (argc >= 4) {
    recvbufsize = atoi(argv[3]);
  }
	if (argc >=5) {
		print = atoi(argv[4]);
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

  if (recvbufsize != 0) {
	  ret = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbufsize, sizeof(recvbufsize));
	  if (ret < 0) ERR_EXIT("setsockopt");
  }
  n = sizeof(recvbufsize);
  ret = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbufsize, &n);
  if (ret < 0) ERR_EXIT("getsockopt");
  printf("actual recvbufsize: %d\n", recvbufsize);

	int on = 1;
	// 如果不设置，收不到 URG 数据
	setsockopt(sockfd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof on);


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

	g_clientfd = clientfd;
	// 哪个进程处理 URG
	//fcntl(clientfd, F_SETOWN, getpid());
	//signal(SIGURG, handler);

  
  // 6. getsockname, 打印新的 socket 绑定的套接字地址 
  puts("6. getsockname");
  cliaddrlen = sizeof(cliaddr);
  ret = getsockname(clientfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
  if (ret < 0) ERR_EXIT("getsockaddr");
  printf("sockaddr: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

  // 7. send data
  while(1) {
    sleep(3);
		if (sockatmark(clientfd)) {
			puts("OOB come in");
		}
    n = read(clientfd, buf, 1048);
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
		printf("%d bytes read, first byte is: %c, last byte is: %c\n", n, buf[0], buf[n-1]);
  }
  close(clientfd);
  close(sockfd);

  return 0;
}
