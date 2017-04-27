#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int count;
	int writesize;
	int recvbuf;
	int showopts;
	int wait;
} g_option;

static int total = 0;

void handler(int sig);
void setopts(int sockfd);
void usage(const char* progname);
void help();

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		usage(argv[0]);
		return 1;
	}

	if (CONTAINS(args, "help")) {
		usage(argv[0]);
		help();
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETINT(args, g_option.count, "count", 2000);
	SETINT(args, g_option.writesize, "writesize", 1400);
	SETINT(args, g_option.recvbuf, "recvbuf", -1);
	SETINT(args, g_option.wait, "wait", -1);
	SETBOOL(args, g_option.showopts, "showopts", 0);


	if (g_option.count < 0) g_option.count = 1;
	if (g_option.writesize < 0) g_option.writesize = 1;
	if (g_option.writesize > 4096) g_option.writesize = 4096;

	registSignal(SIGINT, handler);

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}

  return 0;
}

void server_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	setopts(sockfd);
	if (g_option.showopts) showopts(sockfd, NULL);

	ret = bind(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	doServer(sockfd);

	close(sockfd);
}

void client_routine() {
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	setopts(sockfd);
	if (g_option.showopts) showopts(sockfd, NULL);

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	char buf[4096];
	int nr, nw;
	struct sockaddr_in cliaddr;
	socklen_t len;

	total = 0;
  while(1) {
		if (g_option.wait > 0) usleep(g_option.wait);
		len = sizeof(cliaddr);
		nr = recvfrom(sockfd, buf, 4096, 0, (struct sockaddr*)&cliaddr, &len); 
		if (nr < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("recvfrom");
		}
		++total;
	}
}

void doClient(int sockfd) {
	int ret, len, nr, nw, count;
	struct sockaddr_in servaddr;
	char buf[4096];

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	count = g_option.count;
  while(count--) {
	  nw = sendto(sockfd, buf, g_option.writesize, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));	
		if (nw < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("sendto");
		}
	}
	printf("%d datagrams sended!\n", g_option.count);
}

void handler(int sig) {
	if (sig == SIGINT) {
		printf("received %d datagrams\n", total);
		total = 0;
	}
}

void setopts(int sockfd) {
	if (g_option.recvbuf > 0) {
		printf("set recvbuf: %d\n", g_option.recvbuf);
		setRecvBufSize(sockfd, g_option.recvbuf);
	}
}

void usage(const char* progname) {
	printf("Usage:\n  %s [--help] [-s] [-h hostname] [-p port] "
			"[--count number] [--writesize size] [--recvbuf size] "
			"[--wait time] [--showopts]\n", progname);
}

void help() {
	printf(
			"\t-s                  以服务器方式启动，按 CTRL C 统计接收报文数\n"
			"\t-h                  指定主机名或 ip 地址，默认为通用地址\n"
			"\t-p                  指定端口号，默认为 8000\n"
			"\t--count             客户端发包次数\n"
			"\t--writesize         发送的 udp 数据报大小，不超过 4096\n"
			"\t--recvbuf           设置接收缓冲区大小，默认使用系统指定大小\n"
			"\t--wait              服务器接收完一个数据包后等待多少微秒，默认不等待\n"
			"\t--showopts          打印套接字选项\n");
}
