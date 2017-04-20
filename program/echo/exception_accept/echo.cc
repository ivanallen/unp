#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int isLinger;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] <-h hostname> [-p port] [-r]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETBOOL(args, g_option.isLinger, "r", 0);

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}

  return 0;
}

void server_routine() {
	int ret, listenfd, sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddrlen;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	// 模拟异常，在 accept 前收到 RST 报文
	if (g_option.isLinger) sleep(10);

	cliaddrlen = sizeof cliaddr;
	puts("accepting...");
  sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
	printf("accepted, return %d\n", sockfd);
	if (sockfd < 0) {
		if (errno == ECONNABORTED) puts("accept: connect reset by peer");
		ERR_EXIT("accept");
	}
	printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

	doServer(sockfd);
	close(sockfd);
	close(listenfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;
	struct linger lgr;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	if (g_option.isLinger) {
		lgr.l_onoff = 1;
		lgr.l_linger = 0;
		ret = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lgr, sizeof lgr);
		if (ret < 0) ERR_EXIT("set linger");
	}

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

	// 测试连接成功后立即发送 RST
	if (g_option.isLinger) {
		puts("connect successful, now exiting...");
		close(sockfd);
		return;
	}

	doClient(sockfd);
	close(sockfd);
}

void doServer(int sockfd) {
	int nr, nw;
	char buf[4096];

	while(1) {
		nr = readline(sockfd, buf, 4096);
		if (nr == 0) {
			puts("peer closed");
			break;
		}
		else if (nr < 0) {
			if (errno == ECONNRESET) {
				puts("readline: reset by peer");
				break;
			}
			ERR_EXIT("readline");
		}

		toUpper(buf, nr);

		nw = writen(sockfd, buf, nr);
		if (nw < nr) {
			puts("short write");
		}
	}
}

void doClient(int sockfd) {
	int nr, nw;
	char buf[4096];

	while(fgets(buf, 4096, stdin)) {
		nw = writen(sockfd, buf, strlen(buf));
		if (nw < strlen(buf)) puts("short write");

		nr = readline(sockfd, buf, 4096);
		if (nr == 0) {
			puts("peer closed");
			break;
		}
		else if (nr < 0) ERR_EXIT("readline");

		write(STDOUT_FILENO, buf, nr);
	}
}

