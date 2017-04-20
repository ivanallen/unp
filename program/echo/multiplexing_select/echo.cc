#include "common.h"

struct Option{
	char hostname[32];
	int port;
	int isServer;
}g_option;

void server_routine();
void client_routine();
int doAccept(int listenfd, int clifds[], int n);
void doServer(int sockfd);
void doClient(int sockfd);
int makefdset(int clifds[], int n, fd_set *fds);

int main(int argc, char* argv[]) {
	Args args = parsecmdline(argc, argv);
	if (args.empty() || !CONTAINS(args, "h")) {
		printf("Usage:\n  %s [-s] <-h hostname> [-p port]\n");
	}

	SETSTR(args, g_option.hostname, "h", "0");
	SETINT(args, g_option.port, "p", 8000);
	SETBOOL(args, g_option.isServer, "s", 0);

	if (g_option.isServer) {
		server_routine();
	}
	else {
		client_routine();
	}
	return 0;
}

void server_routine() {
	int i, ret, nready, maxfd, listenfd, sockfd, clifds[FD_SETSIZE];
	struct sockaddr_in servaddr;
	fd_set fds, rfds;

	ret = resolve(g_option.hostname, g_option.port, &servaddr);
	if (ret < 0) ERR_EXIT("resolve");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) ERR_EXIT("socket");

	ret = bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	if (ret < 0) ERR_EXIT("bind");

	ret = listen(listenfd, 5);
	if (ret < 0) ERR_EXIT("listen");

	// initialize clifds array. -1 means empty
	for (i = 0; i < FD_SETSIZE; ++i) clifds[i] = -1;
	clifds[0] = listenfd;


	while(1) {
		maxfd = makefdset(clifds, FD_SETSIZE, &rfds);
    nready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
		if (nready < 0) {
			if (errno == EINTR) continue;
			ERR_EXIT("select");
		}
		if (nready == 0) continue;
    
		if (FD_ISSET(listenfd, &rfds)) {
			ret = doAccept(listenfd, clifds, FD_SETSIZE);
			if (ret < 0) {
				puts("Too many open files!");
			}
			if (--nready <= 0) continue;
		}

		for (i = 0; i < FD_SETSIZE; ++i) {
			if (clifds[i] != -1 && FD_ISSET(clifds[i], &rfds)) {
				doServer(clifds[i]);
				FD_CLR(clifds[i], &fds);
				close(clifds[i]);
				clifds[i] = -1;
        // nready 是发生 IO 事件的个数，每处理一个，就将其减 1，
				// 如果 nready <= 0，表示 IO 事件已经处理完，后面就没必要再遍历了
				if (--nready <= 0) continue;
			}
		}
	}
}

int doAccept(int listenfd, int clifds[], int n) {
	int sockfd, i;
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;

	cliaddrlen = sizeof(cliaddr);
	sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &cliaddrlen);
	if (sockfd == -1) {
		if (errno != EINTR && errno != ECONNABORTED) {
			ERR_EXIT("doAccept");
		}
	}

	printf("%s:%d come in\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	for (i = 0; i < FD_SETSIZE; ++i) {
		if (clifds[i] == -1) { 
			clifds[i] = sockfd;
			break;
		}
	}
	if (i == FD_SETSIZE) {
		close(sockfd);
		errno = EAGAIN;
		return -1;
	}
	return 0;
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

// return maxfd
int makefdset(int clifds[], int n, fd_set *fds) {
	int i, maxfd;

	maxfd = 0;
	FD_ZERO(fds);

	for (i = 0; i < n; ++i) {
		if (clifds[i] != -1) {
      FD_SET(clifds[i], fds);
			if (clifds[i] > maxfd) maxfd = clifds[i];
		}
	}

	return maxfd;
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
			if (errno = ECONNRESET) {
				perror("readline");
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
