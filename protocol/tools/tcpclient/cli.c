#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

#define ERR_EXIT(msg) do { perror(msg); exit(1); } while(0)

int main(int argc, char* argv[]) {
  if (argc < 3) return 1;
  int sockfd, ret, n;
  char buf[64];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
  socklen_t cliaddrlen;
  
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(argv[1]);
  servaddr.sin_port = htons(atoi(argv[2]));

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) ERR_EXIT("socket");

  ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  if (ret < 0) ERR_EXIT("connect");


  while(1) {
    scanf("%s", buf);
    if (buf[0] == 'q') break;
    write(sockfd, buf, strlen(buf));
    n = read(sockfd, buf, 63);
		if (n < 0) {
			if (errno == EINTR) continue;
			perror("read error");
			break;
		}
    buf[n] = 0;
    puts(buf);
  }

  close(sockfd);
  return 0;
}

