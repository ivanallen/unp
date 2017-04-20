#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();

struct Options {
	int isServer;
	char hostname[32];
	int port;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty()){
		printf("Usage:\n  %s [-s] <-h hostname> [-p port]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);

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
	pid_t pid;
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

	while(1) {
		cliaddrlen = sizeof cliaddr;
		sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
		if (sockfd < 0) {
			if (errno == ECONNABORTED) puts("accept: connect reset by peer");
			ERR_EXIT("accept");
		}
		printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

		pid = fork();
		if (pid == 0) {
			// child
			close(listenfd);
			doServer(sockfd);
			close(sockfd);
			exit(0);
		}
		close(sockfd);
	}
	close(listenfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

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
		// 如果对方此时已经关闭，第一次 write 后程序会收到 RST 
		// 继续 write 会引发 SIGPIPE 信号
		nw = writen(sockfd, buf, strlen(buf));
		if (nw < strlen(buf)) puts("short write");

		// 如果在 readline 时，RST 还没收到，它返回 0
		// 如果此时收到了 RST，返回错误，连接被重置 
		nr = readline(sockfd, buf, 4096);
		if (nr == 0) {
			puts("peer closed");
			break;
		}
		else if (nr < 0) ERR_EXIT("readline");

		write(STDOUT_FILENO, buf, nr);
	}
}

