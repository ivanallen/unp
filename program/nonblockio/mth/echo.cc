#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
void handler(int);
void* doClientRecv(void *args);

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
	CURSOR_POS(1, 1);

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
	int nr, nw, maxfd, ret, cliclose, totalsend;
  char *buf;
	pthread_t tid;

	totalsend = 0;
	cliclose = 0;
	// 客户端一次发送 Length 字节
	buf = (char*)malloc(g_option.length);
	assert(buf);

	ret = pthread_create(&tid, NULL, doClientRecv, (void*)sockfd);
	if (ret != 0) {
		errno = ret;
		ERR_EXIT("pthread_create");
	}

	while(1) {
		nr = iread(STDIN_FILENO, buf, g_option.length);
		if (nr < 0) ERR_EXIT("iread from stdin");
		else if (nr == 0) {
			// client no data to send. 
			LOG("stdin closed\n");
			shutdown(sockfd, SHUT_WR);
			break;
		}
		else {
			nw = writen(sockfd, buf, nr);
			if (nw < 0) ERR_EXIT("writen to sockfd");
			totalsend += nw;
		}
	}
	free(buf);
	LOG("send %d totally\n", totalsend);
	pthread_join(tid, NULL);
}

void* doClientRecv(void *args) {
	int nr, length, totalrecv, sockfd = (long)args;
	char *buf;

	totalrecv = 0;
	length = g_option.length;
	buf = (char*)malloc(length);
	assert(buf);

	while(1) {
		nr = iread(sockfd, buf, length);
		if (nr < 0) ERR_EXIT("iread");
		else if (nr == 0) {
			LOG("receive %d totally\n", totalrecv);
			WARNING("server closed\n");
			break;
		}
		else {
			totalrecv += nr;
			writen(STDOUT_FILENO, buf, nr);
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
