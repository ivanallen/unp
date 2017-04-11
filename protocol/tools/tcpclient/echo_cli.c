#include <stdio.h>
#include <termios.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

int nonagle = 0;
int count = 1;

char getch();

int setopt(int sockfd) {
  int ret, flag = 1;
  if (nonagle) {
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    if (ret < 0) {
      perror("set nodelay");
      return 0;
    }
    puts("set nodelay");
  }
  return 1;
}

int main(int argc, char* argv[])
{
  if (argc < 3) {
    printf("Usage: %s <ip> <port> [NONAGLE]\n", argv[0]);
    return 1;
  }
  if (argc >= 4) {
    count = atoi(argv[3]);
  }
  if (argc >= 5) {
    if (!strcmp("NONAGLE", argv[4])) {
      nonagle = 1;
    }
  }


  int sockClient;  //客户端socket
  struct sockaddr_in addrSrv;
  int n, i;
  char recvBuf[64];

  sockClient = socket(AF_INET, SOCK_STREAM, 0); //创建socket
  setopt(sockClient);


  addrSrv.sin_addr.s_addr = inet_addr(argv[1]);
  addrSrv.sin_family = AF_INET;
  addrSrv.sin_port = htons(atoi(argv[2]));
  if (connect(sockClient, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) != 0) { //连接服务器端
    printf("connect error");
    return 1;
  }

  puts("connect success");

  while (1) {
    char c = getch();
    if (c == 'q') break;
    i = count;
    while(i--) send(sockClient, &c, 1, 0); //向服务器端发送数据，连续发送 count 次。
    n = recv(sockClient, recvBuf, 64, 0); //接收服务器端数据
    if (n < 0) {
      perror("read error");
      return 1;
    }
    recvBuf[n] = 0;

    printf("%s", recvBuf);
  }
  close(sockClient); //关闭连接
  return 0;
}


static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}

/* Read 1 character with echo */
char getche(void) 
{
  return getch_(1);
}
