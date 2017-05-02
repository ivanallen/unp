#include "common.h"

void doServer(int);
void doClient(int);
void server_routine();
void client_routine();
int connect_timeo(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int nsec);

struct Options {
	int isServer;
	char hostname[32];
	int port;
	int timeout;
} g_option;

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (CONTAINS(args, "help")){
		printf("Usage:\n  %s [--help] [-s] [-h hostname] [-p port] [--timeo nsec]\n", argv[0]);
		return 1;
	}


	SETBOOL(args, g_option.isServer, "s", 0);
	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETINT(args, g_option.timeout, "timeo", 0);

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
}

void client_routine() {
	int ret, sockfd;
	struct sockaddr_in servaddr;
	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) ERR_EXIT("socket");

	ret = connect_timeo(sockfd, (struct sockaddr*)&servaddr, sizeof servaddr, g_option.timeout);
	if (ret < 0) ERR_EXIT("connect_timeo");

	doClient(sockfd);

	close(sockfd);
}

void doServer(int sockfd) {
}

void doClient(int sockfd) {
}

int connect_timeo(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int nsec) {
	int ret;
	void (*oldfun)(int);
	void connect_alarm(int);
	int sig;

	sig = SIGALRM;
	
	registSignal(sig, connect_alarm, &oldfun);
	
	if (oldfun == SIG_ERR) ERR_EXIT("signal");
	if (alarm(nsec) != 0) {
		fprintf(stderr, "connect_timeo: alarm already set\n");
		exit(1);
	}

	ret = connect(sockfd, addr, addrlen);
	if (ret < 0 && errno == EINTR) {
		errno = ETIMEDOUT;
	}

	alarm(0);

	registSignal(sig, oldfun);

	return ret;
}

void connect_alarm(int) {
	return;
}
