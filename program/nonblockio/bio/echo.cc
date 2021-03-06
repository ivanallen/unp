#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
void handler(int);

const char *dots[3] = {".", "..", "..."};

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int length;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "help")){
		printf("Usage:\n  %s [--help] [-s] [-h hostname] [-p port] [-l length]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETINT(args, g_option.length, "l", 4096);

	registSignal(SIGINT, handler);

	CURSOR_OFF();
	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}
	CURSOR_ON();
  return 0;
}

void server_routine() {
	int ret, listenfd, sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t len;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");
	reuseAddr(listenfd, 1);
	showopts(listenfd, "SO_RCVBUF", stderr);
	showopts(listenfd, "SO_SNDBUF", stderr);

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	len = sizeof(cliaddr);
	sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
	if (sockfd < 0) ERR_EXIT("accept");

	WARNING("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

	doServer(sockfd);
	close(sockfd);
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;
	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");
	CLEAR();
	CURSOR_POS(0, 0);

	showopts(sockfd, "SO_RCVBUF", stderr);
	showopts(sockfd, "SO_SNDBUF", stderr);

	ret = connect(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr);
	if (ret < 0) ERR_EXIT("connect");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
	int nr, nw, total, totalsend, i;
	char buf[4096];

	i = 0;
	total = 0;
	totalsend = 0;

	while(1) {
    nr = iread(sockfd, buf, 4096);
		if (nr < 0) ERR_EXIT("iread");
		else if (nr == 0) {
			CURSOR_DOWN(3);
			ERR_PRINT("client closed\n");
			break;
		}

		toUpper(buf, nr);
		total += nr;
		LOG("received %d bytes totally%s\n", total, dots[i]);
		LOG("ready to send %d bytes%s\n", nr, dots[i]);
		nw = writen(sockfd, buf, nr);
		totalsend += nw;
		LOG("send %d bytes totally%s\n", totalsend, dots[i]);
		CURSOR_UP(3);
		i = (i + 1) % 3;
		if (nw < 0) {
			CURSOR_DOWN(3);
			ERR_EXIT("writen");
		}
	}
}

void doClient(int sockfd) {
	int nr, nw, i, j, maxfd, ret, cliclose, totalsend, totalrecv, actualsend;
  char *buf;
	fd_set rfds, fds;

	i = j = 0;
	totalsend = 0;
	totalrecv = 0;
	actualsend = 0;
	cliclose = 0;
	// 客户端一次发送 Length 字节
	buf = (char*)malloc(g_option.length);
	assert(buf);

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(sockfd, &fds);

	maxfd = sockfd;

	while(1) {
		rfds = fds;
		ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("select");
		}

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			nr = iread(STDIN_FILENO, buf, g_option.length);
			if (nr < 0) ERR_EXIT("iread from stdin");
			else if (nr == 0) {
				// client no data to send. 
        shutdown(sockfd, SHUT_WR);
				cliclose = 1;
				FD_CLR(STDIN_FILENO, &fds);
			}
			else {
				CURSOR_POS(3, 1);
				LOG("ready to send %d bytes%s\n", nr, dots[i]);
				nw = writen(sockfd, buf, nr);
				if (nw < 0) ERR_EXIT("writen to sockfd");
				totalsend += nw;
				CURSOR_POS(7, 1);
				LOG("send %d bytes actually%s\n", nw, dots[i]);
				CURSOR_POS(11, 1);
				LOG("send %d bytes totally%s\n", totalsend, dots[i]);
				i = (i + 1) % 3;
			}
		}

		if (FD_ISSET(sockfd, &rfds)) {
			nr = iread(sockfd, buf, g_option.length);
			totalrecv += nr;
			CURSOR_POS(15, 1);
			LOG("recv %d totally%s\n", totalrecv, dots[j]);
			j = (j + 1) % 3;
			if (nr < 0) ERR_EXIT("iread from sockfd");
			else if (nr == 0) {
				// server no data to send.
				CURSOR_POS(19, 1);
				if (cliclose) {
					WARNING("server closed!\n");
				}
				else {
					ERR_PRINT("server exception!\n");
				}
				break;
			}
			else {
				nw = writen(STDOUT_FILENO, buf, nr);
				if (nw < 0) ERR_EXIT("writen to stdout");
			}
		}
	}
	free(buf);
}

void handler(int sig) {
	if (sig == SIGINT) {
		ERR_PRINT("exited!\n");
		RESET();
		CURSOR_ON();
		exit(0);
	}
}
